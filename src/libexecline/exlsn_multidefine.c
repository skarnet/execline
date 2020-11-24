/* ISC license. */

#include <string.h>
#include <skalibs/sgetopt.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <execline/execline.h>
#include "exlsn.h"

int exlsn_multidefine (int argc, char const **argv, char const *const *envp, exlsn_t *info)
{
  eltransforminfo_t si = ELTRANSFORMINFO_ZERO ;
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  size_t varbase = info->vars.len ;
  size_t valbase = info->values.len ;
  size_t pos = valbase ;
  unsigned int i = 0 ;
  unsigned int max ;
  char const *x ;
  int argc1 ;
  int zeroword = 0, likeread = 0 ;
  si.split = 1 ;
  for (;;)
  {
    int opt = subgetopt_r(argc, argv, "0rNnCcd:", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case '0' : zeroword = 1 ; break ;
      case 'r' : likeread = 1 ; break ;
      case 'N' : si.chomp = 0 ; break ;
      case 'n' : si.chomp = 1 ; break ;
      case 'C' : si.crunch = 1 ; break ;
      case 'c' : si.crunch = 0 ; break ;
      case 'd' : si.delim = localopt.arg ; break ;
      default : return -3 ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;

  if (argc < 2) return -3 ;
  x = argv[0] ;
  argv++ ; argc-- ;
  argc1 = el_semicolon(argv) ;
  if (!argc1) return -4 ;
  if (argc1 >= argc) return -3 ;
  if (!stralloc_cats(&info->values, x)) return -1 ;
  {
    int r = el_transform(&info->values, valbase, &si) ;
    if (r < 0) goto err ;
    max = r ;
  }
  if (!stralloc_0(&info->values)) goto err ;
  for (; i < (unsigned int)argc1 ; i++)
  {
    if (*argv[i])
    {
      elsubst_t blah ;
      blah.var = info->vars.len ;
      if (el_vardupl(argv[i], info->vars.s, info->vars.len)) goto err2 ;
      if (!stralloc_catb(&info->vars, argv[i], strlen(argv[i]) + 1)) goto err ;
      blah.value = i < max ? pos : info->values.len - 1 ;
      blah.n = (i < max) || !zeroword ;
      if (!genalloc_append(elsubst_t, &info->data, &blah)) goto err ;
    }
    if (i < max) pos += strlen(info->values.s + pos) + 1 ;
  }
  if ((i < max) && likeread) genalloc_s(elsubst_t, &info->data)[i-1].n = max - i + 1 ;

  (void)envp ;
  return localopt.ind + argc1 + 2 ;

 err:
  info->vars.len = varbase ;
  info->values.len = valbase ;
  return -1 ;
 err2:
  info->vars.len = varbase ;
  info->values.len = valbase ;
  return -2 ;
}
