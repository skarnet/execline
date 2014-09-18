/* ISC license. */

#include <skalibs/bytestr.h>
#include <skalibs/env.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/uint.h>
#include <execline/execline.h>
#include "exlsn.h"
	
int exlp (unsigned int nmin, char const *const *envp, exlsn_t *info)
{
  unsigned int varbase = info->vars.len ;
  unsigned int valbase = info->values.len ;
  unsigned int datbase = genalloc_len(elsubst_t, &info->data) ;
  unsigned int i = 0 ;
  char const *x = env_get2(envp, "#") ;
  elsubst_t blah ;
  unsigned int n, ntot, poszero ;
  if (!x) return -2 ;
  if (!uint0_scan(x, &n)) return -2 ;
  if (el_vardupl("#", info->vars.s, info->vars.len)) return -2 ;
  if (el_vardupl("@", info->vars.s, info->vars.len)) return -2 ;
  {
    register unsigned int strict = el_getstrict() ;
    if (strict && (n < nmin))
    {
      char fmta[UINT_FMT] ;
      char fmtn[UINT_FMT] ;
      fmta[uint_fmt(fmta, n)] = 0 ;
      fmtn[uint_fmt(fmtn, nmin)] = 0 ;
      if (strict > 1)
        strerr_dief4x(100, "too few arguments: expected at least ", fmtn, " but got ", fmta) ;
      else
        strerr_warnw4x("too few arguments: expected at least ", fmtn, " but got ", fmta) ;
    }
  }
  blah.var = varbase ;
  blah.value = info->values.len ;
  blah.n = 1 ;
  if (!stralloc_catb(&info->vars, "#\0@", 4)
   || !stralloc_catb(&info->values, x, str_len(x) + 1)
   || !genalloc_append(elsubst_t, &info->data, &blah)) goto err ;
  ntot = n > nmin ? n : nmin ;
  poszero = info->values.len ;
  for (; i <= ntot ; i++)
  {
    char fmt[UINT_FMT] ;
    unsigned int l = uint_fmt(fmt, i) ;
    fmt[l] = 0 ;
    if (el_vardupl(fmt, info->vars.s, info->vars.len)) goto err2 ;
    x = (i <= n) ? env_get2(envp, fmt) : "" ;
    if (!x) goto err2 ;
    blah.var = info->vars.len ;
    blah.value = info->values.len ;
    blah.n = 1 ;
    if (!stralloc_catb(&info->vars, fmt, l+1)
     || !stralloc_catb(&info->values, x, str_len(x) + 1)
     || !genalloc_append(elsubst_t, &info->data, &blah)) goto err ;
  }
  blah.var = varbase + 2 ;
  blah.value = poszero + str_len(info->values.s + poszero) + 1 ;
  blah.n = n ;
  if (!genalloc_append(elsubst_t, &info->data, &blah)) goto err ;
  return n ;

 err:
  info->vars.len = varbase ;
  info->values.len = valbase ;
  genalloc_setlen(elsubst_t, &info->data, datbase) ;
  return -1 ;
 err2:
  info->vars.len = varbase ;
  info->values.len = valbase ;
  genalloc_setlen(elsubst_t, &info->data, datbase) ;
  return -2 ;
}
