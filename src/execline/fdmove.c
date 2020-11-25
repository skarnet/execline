/* ISC license. */

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "fdmove [ -c ] to from prog..."

int main (int argc, char const *const *argv)
{
  unsigned int to, from ;
  int flagcopy = 0 ;
  PROG = "fdmove" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "c", &l) ;
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
  if ((flagcopy ? fd_copy(to, from) : fd_move(to, from)) == -1)
    strerr_diefu4sys(111, "move fd ", argv[1], " to fd ", argv[0]) ;
  xexec(argv+2) ;
}
