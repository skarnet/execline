/* ISC license. */

#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "fdswap fd1 fd2 prog..."

int main (int argc, char const *const *argv)
{
  unsigned int fd1, fd2 ;
  PROG = "fdswap" ;
  if ((argc < 4) || !uint0_scan(argv[1], &fd1) || !uint0_scan(argv[2], &fd2))
    strerr_dieusage(100, USAGE) ;
  if (fd_move2(fd1, fd2, fd2, fd1) < 0) strerr_diefu1sys(111, "swap fds") ;
  xexec(argv+3) ;
}
