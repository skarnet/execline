/* ISC license. */

#include <sys/types.h>
#include <sys/wait.h>
#include <skalibs/sgetopt.h>
#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

#define USAGE "wait [ -r ] { pids... }"

static unsigned int waitall (void)
{
  register unsigned int n = 0 ;
  int wstat ;
  while (wait(&wstat) > 0) n++ ;
  return n ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  int argc1 ;
  int flagreap = 0 ;
  PROG = "wait" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "r", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'r' : flagreap = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (!argc1) flagreap ? wait_reap() : waitall() ;
  else
  {
    pid_t tab[argc1] ;
    register unsigned int i = 0 ;
    for (; i < (unsigned int)argc1 ; i++)
    {
      unsigned int pid ;
      if (!uint0_scan(argv[i], &pid)) strerr_dieusage(100, USAGE) ;
      tab[i] = pid ;
    }
    if (flagreap)
    {
      if (waitn_reap(tab, argc1) < 0)
        strerr_diefu1sys(111, "waitn_reap") ;
    }
    else if (!waitn(tab, argc1)) strerr_diefu1sys(111, "waitn") ;
  }
  pathexec0_run(argv + argc1 + 1, envp) ;
  strerr_dieexec(111, argv[argc1 + 1]) ;
}
