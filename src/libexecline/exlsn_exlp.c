/* ISC license. */

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include "exlsn.h"

int exlsn_exlp (int argc, char const **argv, char const *const *envp, exlsn_t *info)
{
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  unsigned int nmin = 0 ;
  int n ;
  for (;;)
  {
    int opt = subgetopt_r(argc, argv, "P:", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'P' : if (uint0_scan(localopt.arg, &nmin)) break ;
      default : return -3 ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;
  n = exlp(nmin, envp, info) ;
  if (n < 0) return n ;
  return localopt.ind ;
}
