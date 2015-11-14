/* ISC license. */

#include <unistd.h>
#include <errno.h>
#include <skalibs/uint.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "backtick [ -i | -D default ] [ -n ] var { prog... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv, char const *const *envp)
{
  char const *def = 0 ;
  int insist = 0, chomp = 0 ;
  PROG = "backtick" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "eniD:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
	case 'e' : break ; /* compat */
        case 'n' : chomp = 1 ; break ;
        case 'i' : insist = 1 ; break ;
        case 'D' : def = l.arg ; break ;
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
    char const *newargv[argc + 15] ;
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
    if (!argv[i]) strerr_dief1x(100, "empty remainder") ;
    newargv[m++] = EXECLINE_BINPREFIX "withstdinas" ;
    if (insist) newargv[m++] = "-i" ;
    if (chomp) newargv[m++] = "-n" ;
    if (def)
    {
      newargv[m++] = "-D" ;
      newargv[m++] = def ;
    }
    newargv[m++] = "-!" ;
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
