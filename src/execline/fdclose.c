/* ISC license. */

#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE ">&- fd prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int fd ;
  PROG = ">&-" ;
  if ((argc < 3) || !uint0_scan(argv[1], &fd)) strerr_dieusage(100, USAGE) ;
  fd_close(fd) ;
  xpathexec_run(argv[2], argv+2, envp) ;
}
