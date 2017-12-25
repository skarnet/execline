/* ISC license. */

#include <stdlib.h>
#include <skalibs/types.h>
#include <execline/execline.h>

unsigned int el_getstrict (void)
{
  static unsigned int strict = 0 ;
  static int first = 1 ;
  if (first)
  {
    char const *x = getenv("EXECLINE_STRICT") ;
    first = 0 ;
    if (x) uint0_scan(x, &strict) ;
  }
  return strict ;
}
