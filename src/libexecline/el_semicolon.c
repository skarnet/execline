/* ISC license. */

#include <skalibs/strerr.h>
#include <skalibs/types.h>

#include <execline/execline.h>

int el_semicolon (char const **argv)
{
  static unsigned int nblock = 0 ;
  unsigned int strict = el_getstrict() ;
  nblock++ ;
  unsigned int i = 0 ;
  for (; argv[i] ; i++)
  {
    if (!argv[i][0]) return i ;
    else if (argv[i][0] == ' ') argv[i]++ ;
    else if (strict)
    {
      char fmt1[UINT_FMT] ;
      char fmt2[UINT_FMT] ;
      fmt1[uint_fmt(fmt1, nblock)] = 0 ;
      fmt2[uint_fmt(fmt2, i)] = 0 ;
      if (strict >= 2)
        strerr_dief6x(100, "unquoted argument ", argv[i], " at block ", fmt1, " position ", fmt2) ;
      else
        strerr_warnw6x("unquoted argument ", argv[i], " at block ", fmt1, " position ", fmt2) ;
    }
  }
  return i + 1 ;
}
