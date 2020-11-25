/* ISC license. */

#include <string.h>

#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#define USAGE "export variable value prog..."

int main (int argc, char const *const *argv)
{
  size_t len1 ;
  PROG = "export" ;
  if (argc < 4) strerr_dieusage(100, USAGE) ;
  len1 = strlen(argv[1]) ;
  if (memchr(argv[1], '=', len1))
    strerr_dief2x(100, "invalid variable name: ", argv[1]) ;
  {
    size_t len2 = strlen(argv[2]) ;
    char fmt[len1 + len2 + 2] ;
    memcpy(fmt, argv[1], len1) ;
    fmt[len1] = '=' ;
    memcpy(fmt + len1 + 1, argv[2], len2 + 1) ;
    xmexec_n(argv+3, fmt, len1 + len2 + 2, 1) ;
  }
}
