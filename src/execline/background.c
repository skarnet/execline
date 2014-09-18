/* ISC license. */

#include <sys/types.h>
#include <unistd.h>
#ifdef EXECLINE_OLD_VARNAMES
#include <skalibs/bytestr.h>
#endif
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>
#include <skalibs/uint64.h>
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
      register int opt = subgetopt_r(argc, argv, "d", &l) ;
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
        pathexec0_run(argv, envp) ;
        strerr_dieexec(127, argv[0]) ;
    }
  }
  else
  {
    pid = el_spawn0(argv[0], argv, envp) ;
    if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
  }
  if (argc1 + 1 == argc) return 0 ;
  {
#ifdef EXECLINE_OLD_VARNAMES
    char fmt[UINT64_FMT * 2 + 10] = "!=" ;
#else
    char fmt[UINT64_FMT + 2] = "!=" ;
#endif
    register unsigned int i = 2 ;
    i += uint64_fmt(fmt+i, (uint64)pid) ; fmt[i++] = 0 ;
#ifdef EXECLINE_OLD_VARNAMES
    byte_copy(fmt+i, 8, "LASTPID=") ; i += 8 ;
    i += uint64_fmt(fmt+i, (uint64)pid) ; fmt[i++] = 0 ;
#endif
    pathexec_r(argv + argc1 + 1, envp, env_len(envp), fmt, i) ;
  }
  strerr_dieexec(111, argv[argc1+1]) ;
}
