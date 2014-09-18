/* ISC license. */

#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE "fdclose fd prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int fd ;
  PROG = "fdclose" ;
  if ((argc < 3) || !uint0_scan(argv[1], &fd)) strerr_dieusage(100, USAGE) ;
  fd_close((int)fd) ;
  pathexec_run(argv[2], argv+2, envp) ;
  strerr_dieexec(111, argv[2]) ;
}
