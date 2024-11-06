/* ISC license. */

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/exec.h>

#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forbacktickx [ -p | -P maxpar | -o okcode,okcode,... | -x breakcode,breakcode,... ] [ -E | -e ] [ -N | -n ] [ -C | -c ] [ -0 | -d delim ] var { backtickcmd... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  char const *delim = "\n" ;
  char const *codes = 0 ;
  unsigned int maxpar = 1 ;
  int crunch = 0, chomp = 1, not = 1, doimport = 0 ;
  PROG = "forbacktickx" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "pP:NnCc0d:o:x:Ee", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' : maxpar = 0 ; break ;
        case 'P' : if (!uint0_scan(l.arg, &maxpar)) dieusage() ; break ;
        case 'N' : chomp = 0 ; break ;
        case 'n' : chomp = 1 ; break ;
        case 'C' : crunch = 1 ; break ;
        case 'c' : crunch = 0 ; break ;
        case '0' : delim = 0 ; break ;
        case 'd' : delim = l.arg ; break ;
        case 'o' :
        {
          unsigned short okcodes[256] ;
          size_t nbc ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          codes = l.arg ;
          not = 0 ;
          break ;
        }
        case 'x' :
        {
          unsigned short okcodes[256] ;
          size_t nbc ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          codes = l.arg ;
          not = 1 ;
          break ;
        }
        case 'E' : doimport = 1 ; break ;
        case 'e' : doimport = 0 ; break ;
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
    char const *newargv[argc + 20] ;
    char fmt[UINT_FMT] ;
    char fmtmaxpar[UINT_FMT] ;
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
    newargv[m++] = doimport ? "-E" : "-e" ;
    if (!maxpar) newargv[m++] = "-p" ;
    else if (maxpar > 1)
    {
      newargv[m++] = "-P" ;
      newargv[m++] = fmtmaxpar ;
      fmtmaxpar[uint_fmt(fmtmaxpar, maxpar)] = 0 ;
    }
    newargv[m++] = chomp ? "-n" : "-N" ;
    if (crunch) newargv[m++] = "-C" ;
    if (!delim) newargv[m++] = "-0" ;
    else if (strcmp(delim, "\n"))
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
    xexec(newargv) ;
  }
}
