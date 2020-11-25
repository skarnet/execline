/* ISC license. */

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "fdblock [ -n ] fd prog..."

int main (int argc, char const *const *argv)
{
  unsigned int fd ;
  int block = 1 ;
  PROG = "fdblock" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "n", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : block = 0 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if ((argc < 2) || !uint0_scan(argv[0], &fd)) strerr_dieusage(100, USAGE) ;
  if ((block ? ndelay_off(fd) : ndelay_on(fd)) < 0)
    strerr_diefu1sys(111, block ? "ndelay_off" : "ndelay_on") ;
  xexec(argv+1) ;
}
