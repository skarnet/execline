/* ISC license. */

#include <stdlib.h>

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/env.h>
#include <skalibs/exec.h>
#include <skalibs/skamisc.h>

#include <execline/execline.h>

#define USAGE "elgetopt [ -D default ] optstring prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  size_t envlen = env_len(envp) ;
  stralloc modif = STRALLOC_ZERO ;
  char const *x = getenv("#") ;
  char const *dfl = "1" ;
  unsigned int n, nbak ;

  PROG = "elgetopt" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "D:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'D' : dfl = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) strerr_dieusage(100, USAGE) ;
  if (!x) strerr_dienotset(100, "#") ;
  if (!uint0_scan(x, &n)) strerr_dieinvalid(100, "#") ;
  nbak = n++ ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    char const *args[n+1] ;
    unsigned int i = 0 ;
    for ( ; i < n ; i++)
    {
      char fmt[UINT_FMT] ;
      fmt[uint_fmt(fmt, i)] = 0 ;
      args[i] = getenv(fmt) ;
      if (!args[i]) strerr_dienotset(100, fmt) ;
    }
    args[n] = 0 ;
    for (;;)
    {
      char hmpf[11] = "ELGETOPT_?" ;
      int opt = sgetopt_r(n, args, argv[0], &l) ;
      if (opt == -1) break ;
      if (opt == '?') return 1 ;
      hmpf[9] = opt ;
      if (!env_addmodif(&modif, hmpf, l.arg ? l.arg : dfl)) goto err ;
    }
    n -= l.ind ;
    for (i = 0 ; i < nbak ; i++)
    {
      char fmt[UINT_FMT]  ;
      fmt[uint_fmt(fmt, i+1)] = 0 ;
      if (!env_addmodif(&modif, fmt, (i < n) ? args[l.ind + i] : 0)) goto err ;
    }
  }
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, n)] = 0 ;
    if (!env_addmodif(&modif, "#", fmt)) goto err ;
  }
  {
    char const *const list[1] = { "ELGETOPT_" } ;
    char const *v[envlen] ;
    if (el_pushenv(&satmp, envp, envlen, list, 1) < 0) goto err ;
    if (!env_make(v, envlen, satmp.s, satmp.len)) goto err ;
    xmexec_fm(argv+1, v, envlen, modif.s, modif.len) ;
  }
err:
  strerr_diefu1sys(111, "update environment") ;
}
