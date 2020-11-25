/* ISC license. */

#include <sys/wait.h>

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "ifelse [ -n ] [ -X ] { command-if } { command-then... } command-else..."

int main (int argc, char const **argv, char const *const *envp)
{
  int argc1, argc2, wstat ;
  int not = 0, flagnormalcrash = 0 ;
  pid_t pid ;
  PROG = "ifelse" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nX", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : not = 1 ; break ;
        case 'X' : flagnormalcrash = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated if block") ;
  if (argc1 + 1 == argc) strerr_dief1x(100, "then block required") ;
  argc2 = el_semicolon(argv + argc1 + 1) ;
  if (argc1 + argc2 + 1 >= argc) strerr_dief1x(100, "unterminated then block") ;
  argv[argc1] = 0 ;
  pid = el_spawn0(argv[0], argv, envp) ;
  if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
  if (wait_pid(pid, &wstat) == -1)
    strerr_diefu2sys(111, "wait for ", argv[0]) ;
  argv += ++argc1 ;
  if (!flagnormalcrash && WIFSIGNALED(wstat))
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, WTERMSIG(wstat))] = 0 ;
    strerr_dief2x(128 + WTERMSIG(wstat), "child crashed with signal ", fmt) ;
  }
  if (not != !wait_estatus(wstat)) argv[argc2] = 0 ; else argv += argc2+1 ;
  xexec0_e(argv, envp) ;
}
