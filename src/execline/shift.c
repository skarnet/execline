/* ISC license. */

#include <stdlib.h>

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "shift [ -n arg# ] [ -b block# ] prog..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  unsigned int strict = el_getstrict() ;
  unsigned int b = 0 ;
  unsigned int n = 0 ;
  unsigned int sharp ;
  unsigned int i = 0 ;
  PROG = "shift" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "n:b:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' :
          if (!uint0_scan(l.arg, &n)) dieusage() ;
          i = 1 ;
          break ;
        case 'b' :
          if (!uint0_scan(l.arg, &b)) dieusage() ;
          i = 1 ;
          break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) dieusage() ;
  if (i) i = 0 ; else n = 1 ;
  {
    char const *x = getenv("#") ;
    if (!x) strerr_dienotset(100, "#") ;
    if (!uint0_scan(x, &sharp)) strerr_dieinvalid(100, "#") ;
  }


 /* Shift n args */

  if (n > sharp)
  {
    if (strict)
    {
      char fmtn[UINT_FMT] ;
      char fmtsharp[UINT_FMT] ;
      fmtn[uint_fmt(fmtn, n)] = 0 ;
      fmtsharp[uint_fmt(fmtsharp, sharp)] = 0 ;
      if (strict == 1)
        strerr_warnwu5x("shift", " ", fmtn, " arguments: got ", fmtsharp) ;
      else
        strerr_diefu5x(100, "shift", " ", fmtn, " arguments: got ", fmtsharp) ;
    }
    n = sharp ;
  }


 /* Shift b blocks */

  for (; i < b ; i++)
  {
    for (;;)
    {
      char const *x ;
      unsigned int base = n ;
      char fmt[UINT_FMT] ;
      fmt[uint_fmt(fmt, ++n)] = 0 ;
      if (n > sharp)
      {
        char fmti[UINT_FMT] ;
        fmti[uint_fmt(fmt, i)] = 0 ;
        strerr_diefu6x(100, "shift", " block ", fmti, ": too few arguments (", fmt, ")") ;
      }
      x = getenv(fmt) ;
      if (!x) strerr_dienotset(100, fmt) ;
      if ((x[0] == EXECLINE_BLOCK_END_CHAR) && (!EXECLINE_BLOCK_END_CHAR || !x[1])) break ;
      if ((x[0] != EXECLINE_BLOCK_QUOTE_CHAR) && strict)
      {
        char fmti[UINT_FMT] ;
        char fmtp[UINT_FMT] ;
        fmti[uint_fmt(fmti, i)] = 0 ;
        fmtp[uint_fmt(fmtp, n - base)] = 0 ;
        if (strict == 1)
          strerr_warnw6x("unquoted positional ", x, " at block ", fmti, " position ", fmtp) ;
        else
          strerr_dief6x(100, "unquoted positional ", x, " at block ", fmti, " position ", fmtp) ;
      }
    }
  }


 /* n = shift value; modify the env */

  {
    unsigned int i = 1 ;
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, sharp - n)] = 0 ;
    if (!env_mexec("#", fmt)) strerr_diefu1sys(111, "env_mexec") ;
    for (; i <= sharp ; i++)
    {
      char fmu[UINT_FMT] ;
      fmt[uint_fmt(fmt, i)] = 0 ;
      fmu[uint_fmt(fmu, i + n)] = 0 ;
      if (!env_mexec(fmt, i <= (sharp - n) ? getenv(fmu) : 0))
        strerr_diefu1sys(111, "modify environment") ;
    }
  }
  xmexec(argv) ;
}
