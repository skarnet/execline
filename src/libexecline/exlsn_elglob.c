/* ISC license. */

#include <errno.h>
#include <glob.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <execline/execline.h>
#include "exlsn.h"

static int elgloberrfunc (char const *s, int e)
{
  errno = e ;
  strerr_warnw2sys("while globbing, error reading ", s) ;
  return 0 ;
}

int exlsn_elglob (int argc, char const **argv, char const *const *envp, exlsn_t *info)
{
  glob_t pglob ;
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  elsubst_t blah ;
  int flags = GLOB_NOSORT | GLOB_NOCHECK ;
  unsigned int i = 0 ;
  int verbose = 0 ;
  blah.var = info->vars.len ;
  blah.value = info->values.len ;
  for (;;)
  {
    register int opt = subgetopt_r(argc, argv, "vwsme0", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'v' : verbose = 1 ; break ;
      case 'w' : flags |= GLOB_ERR ; break ;
      case 's' : flags &= ~GLOB_NOSORT ; break ;
      case 'm' : flags |= GLOB_MARK ; break ;
      case 'e' : flags |= GLOB_NOESCAPE ; break ;
      case '0' : flags &= ~GLOB_NOCHECK ; break ;
      default : return -3 ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;

  if (argc < 2) return -3 ;
  if (!*argv[0] || el_vardupl(argv[0], info->vars.s, info->vars.len)) return -2 ;
  if (!stralloc_catb(&info->vars, argv[0], str_len(argv[0]) + 1)) return -1 ;

  pglob.gl_offs = 0 ;
  switch (glob(argv[1], flags, verbose ? &elgloberrfunc : 0, &pglob))
  {
    case 0 : break ;
    case GLOB_NOMATCH:
    {
      pglob.gl_pathc = 0 ;
      pglob.gl_pathv = 0 ;
      break ;
    }
    default: goto err ;
  }
  for ( ; i < (unsigned int)pglob.gl_pathc ; i++)
    if (!stralloc_catb(&info->values, pglob.gl_pathv[i], str_len(pglob.gl_pathv[i]) + 1))
      goto globerr ;
  blah.n = pglob.gl_pathc ;
  globfree(&pglob) ;
  if (!genalloc_append(elsubst_t, &info->data, &blah)) goto err ;
  (void)envp ;
  return localopt.ind + 2 ;

 globerr:
  globfree(&pglob) ;
 err:
  info->vars.len = blah.var ;
  info->values.len = blah.value ;
  return -1 ;
}
