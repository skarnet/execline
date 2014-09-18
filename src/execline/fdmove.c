/* ISC license. */

#include <skalibs/sgetopt.h>
#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE "fdmove [ -c ] to from prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int to, from ;
  int flagcopy = 0 ;
  PROG = "fdmove" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "c", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'c' : flagcopy = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if ((argc < 3) || !uint0_scan(argv[0], &to) || !uint0_scan(argv[1], &from))
    strerr_dieusage(100, USAGE) ;
  if ((flagcopy ? fd_copy((int)to, (int)from) : fd_move((int)to, (int)from)) == -1)
    strerr_diefu4sys(111, "move fd ", argv[1], " to fd ", argv[0]) ;
  pathexec_run(argv[2], argv+2, envp) ;
  strerr_dieexec(111, argv[2]) ;
}
