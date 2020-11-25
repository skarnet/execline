/* ISC license. */

#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "fdclose fd prog..."

int main (int argc, char const *const *argv)
{
  unsigned int fd ;
  PROG = "fdclose" ;
  if ((argc < 3) || !uint0_scan(argv[1], &fd)) strerr_dieusage(100, USAGE) ;
  fd_close(fd) ;
  xexec(argv+2) ;
}
