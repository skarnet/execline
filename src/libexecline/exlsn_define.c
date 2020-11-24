/* ISC license. */

#include <string.h>
#include <skalibs/sgetopt.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <execline/execline.h>
#include "exlsn.h"

int exlsn_define (int argc, char const **argv, char const *const *envp, exlsn_t *info)
{
  eltransforminfo_t si = ELTRANSFORMINFO_ZERO ;
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  elsubst_t blah ;
  blah.var = info->vars.len ;
  blah.value = info->values.len ;
  for (;;)
  {
    int opt = subgetopt_r(argc, argv, "NnsCcd:", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'N' : si.chomp = 0 ; break ;
      case 'n' : si.chomp = 1 ; break ;
      case 's' : si.split = 1 ; break ;
      case 'C' : si.crunch = 1 ; break ;
      case 'c' : si.crunch = 0 ; break ;
      case 'd' : si.delim = localopt.arg ; break ;
      default : return -3 ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;
 
  if (argc < 2) return -3 ;
  if (!*argv[0] || el_vardupl(argv[0], info->vars.s, info->vars.len)) return -2 ;
  if (!stralloc_catb(&info->vars, argv[0], strlen(argv[0]) + 1)) return -1 ;
  if (!stralloc_cats(&info->values, argv[1])) goto err ;
  {
    int r = el_transform(&info->values, blah.value, &si) ;
    if (r < 0) goto err ;
    blah.n = r ;
  }
  if (!genalloc_append(elsubst_t, &info->data, &blah)) goto err ;
  (void)envp ;
  return localopt.ind + 2 ;

 err:
  info->vars.len = blah.var ;
  info->values.len = blah.value ;
  return -1 ;
}
