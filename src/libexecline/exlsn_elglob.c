/* ISC license. */

#include <sys/uio.h>
#include <errno.h>
#include <glob.h>
#include <string.h>

#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/netstring.h>

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
  elsubst_t blah = { .var = info->vars.len, .value = info->values.len, .n = 1 } ;
  int flags = GLOB_NOSORT | GLOB_NOCHECK ;
  int verbose = 0 ;
  int dochomp = 0 ;
  char const *delim = 0 ;
  subgetopt localopt = SUBGETOPT_ZERO ;
  for (;;)
  {
    int opt = subgetopt_r(argc, argv, "vwsme0nd:", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'v' : verbose = 1 ; break ;
      case 'w' : flags |= GLOB_ERR ; break ;
      case 's' : flags &= ~GLOB_NOSORT ; break ;
      case 'm' : flags |= GLOB_MARK ; break ;
      case 'e' : flags |= GLOB_NOESCAPE ; break ;
      case '0' : flags &= ~GLOB_NOCHECK ; break ;
      case 'n' : dochomp = 1 ; break ;
      case 'd' : delim = localopt.arg ; break ;
      default : return -3 ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;
  if (!delim) delim = "" ; else if (!*delim) delim = 0 ;

  if (argc < 2) return -3 ;
  if (!*argv[0] || el_vardupl(argv[0], info->vars.s, info->vars.len)) return -2 ;
  if (!stralloc_catb(&info->vars, argv[0], strlen(argv[0]) + 1)) return -1 ;

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
  for (size_t i = 0 ; i < pglob.gl_pathc ; i++)
    if (delim ? !stralloc_cats(&info->values, pglob.gl_pathv[i]) || !stralloc_catb(&info->values, delim, 1) :
                !netstring_appendb(&info->values, pglob.gl_pathv[i], strlen(pglob.gl_pathv[i])))
      goto globerr ;
  if (delim && *delim && dochomp) info->values.len-- ;
  if (delim && !*delim) blah.n = pglob.gl_pathc ;
  else if (!stralloc_0(&info->values)) goto globerr ;
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
