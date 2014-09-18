/* ISC license. */

#include <skalibs/djbunix.h>
#include <skalibs/env.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <execline/execline.h>
#include "exlsn.h"

void el_substandrun_str (stralloc *src, unsigned int srcbase, char const *const *envp, exlsn_t *info)
{
  stralloc dst = STRALLOC_ZERO ;
  register int r = el_substitute(&dst, src->s + srcbase, src->len, info->vars.s, info->values.s, genalloc_s(elsubst_t, &info->data), genalloc_len(elsubst_t, &info->data)) ;
  if (r < 0) strerr_diefu1sys(111, "el_substitute") ;
  exlsn_free(info) ;
  stralloc_free(src) ;
  {
    char const *v[r + 1] ;
    if (!env_make(v, r, dst.s, dst.len)) strerr_diefu1sys(111, "env_make") ;
    v[r] = 0 ;
    pathexec0_run(v, envp) ;
  }
  strerr_dieexec(111, dst.s) ;
}
