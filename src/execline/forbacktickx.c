/* ISC license. */

#include <unistd.h>
#include <errno.h>
#include <skalibs/ushort.h>
#include <skalibs/uint.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forbacktickx [ -p | -o okcode,okcode,... | -x breakcode,breakcode,... ] [ -n ] [ -C | -c ] [ -0 | -d delim ] var { backtickcmd... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

#define DELIM_DEFAULT " \n\r\t"

int main (int argc, char const *const *argv, char const *const *envp)
{
  char const *delim = DELIM_DEFAULT ;
  char const *codes = 0 ;
  int crunch = 0, chomp = 0, not = 1, par = 0 ;
  PROG = "forbacktickx" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "epnCc0d:o:x:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
	case 'e' : break ; /* compat */
        case 'p' : par = 1 ; break ;
        case 'n' : chomp = 1 ; break ;
        case 'C' : crunch = 1 ; break ;
        case 'c' : crunch = 0 ; break ;
        case '0' : delim = 0 ; break ;
        case 'd' : delim = l.arg ; break ;
        case 'o' :
        {
          unsigned short okcodes[256] ;
          unsigned int nbc ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          codes = l.arg ;
          not = 0 ;
          break ;
        }
        case 'x' :
        {
          unsigned short okcodes[256] ;
          unsigned int nbc ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          codes = l.arg ;
          not = 1 ;
          break ;
        }
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) dieusage() ;
  if (!argv[0][0]) dieusage() ;
  if (!argv[1][0]) strerr_dief1x(100, "empty block") ;
  {
    unsigned int m = 0, i = 1 ;
    int fd = dup(0) ;
    char const *newargv[argc + 18] ;
    char fmt[UINT_FMT] ;
    if (fd < 0)
    {
      if (errno != EBADF) strerr_diefu1sys(111, "dup stdin") ;
    }
    else fmt[uint_fmt(fmt, (unsigned int)fd)] = 0 ;
    newargv[m++] = EXECLINE_BINPREFIX "pipeline" ;
    newargv[m++] = "--" ;
    while (argv[i] && argv[i][0] != EXECLINE_BLOCK_END_CHAR && (!EXECLINE_BLOCK_END_CHAR || (argv[i][0] && argv[i][1])))
      newargv[m++] = argv[i++] ;
    if (!argv[i]) strerr_dief1x(100, "unterminated block") ;
    newargv[m++] = "" ; i++ ;
    newargv[m++] = EXECLINE_BINPREFIX "unexport" ;
    newargv[m++] = "!" ;
    newargv[m++] = EXECLINE_BINPREFIX "forstdin" ;
    if (par) newargv[m++] = "-p" ;
    if (chomp) newargv[m++] = "-n" ;
    if (crunch) newargv[m++] = "-C" ;
    if (!delim) newargv[m++] = "-0" ;
    else if (str_diff(delim, DELIM_DEFAULT))
    {
      newargv[m++] = "-d" ;
      newargv[m++] = delim ;
    }
    if (codes)
    {
      newargv[m++] = not ? "-x" : "-o" ;
      newargv[m++] = codes ;
    }
    newargv[m++] = "--" ;
    newargv[m++] = argv[0] ;
    if (fd < 0)
    {
      newargv[m++] = EXECLINE_BINPREFIX "fdclose" ;
      newargv[m++] = "0" ;
    }
    else
    {
      newargv[m++] = EXECLINE_BINPREFIX "fdmove" ;
      newargv[m++] = "0" ;
      newargv[m++] = fmt ;
    }
    while (argv[i]) newargv[m++] = argv[i++] ;
    newargv[m++] = 0 ;
    pathexec_run(newargv[0], newargv, envp) ;
    strerr_dieexec(111, newargv[0]) ;
  }
}
