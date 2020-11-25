/* ISC license. */

#include <unistd.h>

#include <skalibs/exec.h>
#include <skalibs/env.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>

#include <execline/execline.h>
#include "exlsn.h"

void el_substandrun_str (stralloc *src, size_t srcbase, char const *const *envp, exlsn_t const *info)
{
  stralloc dst = STRALLOC_ZERO ;
  int r = el_substitute(&dst, src->s + srcbase, src->len, info->vars.s, info->values.s, genalloc_s(elsubst_t const, &info->data), genalloc_len(elsubst_t const, &info->data)) ;
  if (r < 0) strerr_diefu1sys(111, "el_substitute") ;
  if (!r) _exit(0) ;
  stralloc_free(src) ;
  {
    char const *v[r + 1] ;
    if (!env_make(v, r, dst.s, dst.len)) strerr_diefu1sys(111, "env_make") ;
    v[r] = 0 ;
    xmexec_em(v, envp, info->modifs.s, info->modifs.len) ;
  }
}
