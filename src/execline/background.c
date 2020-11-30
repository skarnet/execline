/* ISC license. */

#include <skalibs/posixplz.h>
#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "background [ -d ] { command... }"

int main (int argc, char const **argv, char const *const *envp)
{
  pid_t pid ;
  int argc1 ;
  int df = 0 ;
  PROG = "background" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "d", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'd' : df = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (argc1 + 1 == argc) df = 0 ;
  argv[argc1] = 0 ;

  if (df)
  {
    pid = doublefork() ;
    switch (pid)
    {
      case -1: strerr_diefu1sys(111, "doublefork") ;
      case 0:
        PROG = "background (grandchild)" ;
        xexec0_e(argv, envp) ;
    }
  }
  else
  {
    pid = el_spawn0(argv[0], argv, envp) ;
    if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
  }
  if (argc1 + 1 == argc) return 0 ;
  {
    char fmt[PID_FMT + 2] = "!=" ;
    size_t i = 2 ;
    i += pid_fmt(fmt+i, pid) ; fmt[i++] = 0 ;
    xmexec_en(argv + argc1 + 1, envp, fmt, i, 1) ;
  }
}
