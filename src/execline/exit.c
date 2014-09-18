/* ISC license. */

#include <skalibs/strerr2.h>
#include <skalibs/uint.h>

#define USAGE "exit [ exitcode ]"

int main (int argc, char const *const *argv)
{
  unsigned int e ;
  PROG = "exit" ;
  if (argc < 2) return 0 ;
  if (!uint0_scan(argv[1], &e)) strerr_dieusage(100, USAGE) ;
  return (int)e ;
}
