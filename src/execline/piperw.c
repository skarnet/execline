/* ISC license. */

#include <unistd.h>

#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "piperw fdr fdw prog..."

int main (int argc, char const *const *argv)
{
  int fdr, fdw ;
  int p[2] ;
  PROG = "piperw" ;
  if ((argc < 4)
   || !uint0_scan(argv[1], (unsigned int *)&fdr)
   || !uint0_scan(argv[2], (unsigned int *)&fdw)
   || (fdr == fdw))
    strerr_dieusage(100, USAGE) ;
  if (pipe(p) == -1)
    strerr_diefu1sys(111, "create pipe") ;
  if (p[1] == fdr) p[1] = dup(p[1]) ;
  if ((p[1] == -1)
   || (fd_move(fdr, p[0]) == -1)
   || (fd_move(fdw, p[1]) == -1))
    strerr_diefu1sys(111, "move fds") ;
  xexec(argv+3) ;
}
