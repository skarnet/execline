/* ISC license. */

#include <string.h>
#include <stdlib.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/types.h>
#include <skalibs/netstring.h>

#define USAGE "dollarat [ -n ] [ -0 | -d delimchar ]"

int main (int argc, char const *const *argv)
{
  unsigned int n, i = 0 ;
  char const *x ;
  char delim = '\n' ;
  int zero = 0 ;
  int nl = 1 ;
  PROG = "dollarat" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nd:0", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nl = 0 ; break ;
        case 'd' : delim = *l.arg ; zero = 0 ; break ;
        case '0' : zero = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (zero) delim = 0 ;
  x = getenv("#") ;
  if (!x) strerr_dienotset(100, "#") ;
  if (!uint0_scan(x, &n)) strerr_dieinvalid(100, "#") ;
  for (; i < n ; i++)
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, i+1)] = 0 ;
    x = getenv(fmt) ;
    if (!x) strerr_dienotset(100, fmt) ;
    if (delim || zero)
    {
      if ((buffer_puts(buffer_1, x) < 0)
       || (((i < n-1) || nl) && (buffer_put(buffer_1, &delim, 1) < 0)))
        strerr_diefu1sys(111, "write to stdout") ;
    }
    else
    {
      size_t written = 0 ;
      if (!netstring_put(buffer_1, x, strlen(x), &written))
        strerr_diefu1sys(111, "write a netstring to stdout") ;
    }
  }
  if (!buffer_flush(buffer_1))
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
