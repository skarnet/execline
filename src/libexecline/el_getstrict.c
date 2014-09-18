/* ISC license. */

#include <skalibs/env.h>
#include <skalibs/uint.h>
#include <execline/execline.h>

unsigned int el_getstrict (void)
{
  static unsigned int strict = 0 ;
  static int first = 1 ;
  if (first)
  {
    char const *x = env_get("EXECLINE_STRICT") ;
    first = 0 ;
    if (x) uint0_scan(x, &strict) ;
  }
  return strict ;
}
