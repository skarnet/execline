/* ISC license. */

#include <skalibs/env.h>
#include <skalibs/strerr2.h>
#include <skalibs/uint.h>
#include <execline/execline.h>

int el_semicolon (char const **argv)
{
  static unsigned int nblock = 0 ;
  register int argc1 = 0 ;
  nblock++ ;
  for (;; argc1++, argv++)
  {
    register char const *arg = *argv ;
    if (!arg) return argc1 + 1 ;
    if ((arg[0] == EXECLINE_BLOCK_END_CHAR) && (!EXECLINE_BLOCK_END_CHAR || !arg[1])) return argc1 ;
    else if (arg[0] == EXECLINE_BLOCK_QUOTE_CHAR) ++*argv ;
    else
    {
      unsigned int strict = el_getstrict() ;
      if (strict)
      {
        char fmt1[UINT_FMT] ;
        char fmt2[UINT_FMT] ;
        fmt1[uint_fmt(fmt1, nblock)] = 0 ;
        fmt2[uint_fmt(fmt2, (unsigned int)argc1)] = 0 ;
        if (strict >= 2)
          strerr_dief6x(100, "unquoted argument ", arg, " at block ", fmt1, " position ", fmt2) ;
        else
          strerr_warnw6x("unquoted argument ", arg, " at block ", fmt1, " position ", fmt2) ;
      }
    }
  }
}
