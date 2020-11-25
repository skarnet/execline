/* ISC license. */

#include <string.h>

#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/exec.h>

#define USAGE "unexport variable prog..."

int main (int argc, char const *const *argv)
{
  size_t len ;
  PROG = "unexport" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  len = strlen(argv[1]) ;
  if (memchr(argv[1], '=', len))
    strerr_dief2x(100, "invalid variable name: ", argv[1]) ;
  xmexec_n(argv+2, argv[1], len+1, 1) ;
}
