/* ISC license. */

#include <sys/stat.h>

#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#define USAGE "umask value prog..."

int main (int argc, char const *const *argv)
{
  unsigned int m ;
  PROG = "execline-umask" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  if (!uint_oscan(argv[1], &m)) strerr_dieusage(100, USAGE) ;
  umask(m) ;
  xexec(argv+2) ;
}
