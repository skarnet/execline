/* ISC license. */

#include <sys/types.h>
#include <sys/wait.h>
#include <skalibs/sgetopt.h>
#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/ushort.h>
#include <execline/execline.h>

#define USAGE "if [ -n ] [ -X ] [ -t | -x exitcode ] { command... }"

int main (int argc, char const **argv, char const *const *envp)
{
  int argc1, wstat ;
  pid_t pid ;
  int not = 0, flagnormalcrash = 0 ;
  unsigned short e = 1 ;
  PROG = "if" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "nXtx:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : not = 1 ; break ;
        case 'X' : flagnormalcrash = 1 ; break ;
        case 't' : e = 0 ; break ;
        case 'x' : if (ushort_scan(l.arg, &e)) break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (e > 255) strerr_dief1x(100, "invalid exit code") ;
  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  argv[argc1] = 0 ;
  pid = el_spawn0(argv[0], argv, envp) ;
  if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
  if (wait_pid(pid, &wstat) == -1) strerr_diefu1sys(111, "wait_pid") ;
  if (!flagnormalcrash && WIFSIGNALED(wstat))
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, WTERMSIG(wstat))] = 0 ;
    strerr_dief2x(128 + WTERMSIG(wstat), "child crashed with signal ", fmt) ;
  }
  if (not == !wait_estatus(wstat)) return e ;
  pathexec0_run(argv+argc1+1, envp) ;
  strerr_dieexec(111, argv[argc1+1]) ;
}
