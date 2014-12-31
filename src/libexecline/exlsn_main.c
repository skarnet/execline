/* ISC license. */

#include <skalibs/bytestr.h>
#include <skalibs/strerr2.h>
#include <skalibs/genalloc.h>
#include <execline/execline.h>
#include "exlsn.h"

void exlsn_main (int argc, char const **argv, char const *const *envp, exlsnfunc_t *func, char const *usage)
{
  exlsn_t info = EXLSN_ZERO ;
  int r = (*func)(argc, argv, envp, &info) ;
  if (r < 0) switch (r)
  {
    case -3 : strerr_dieusage(100, usage) ;
    case -2 : strerr_dief1x(111, "bad substitution key") ;
    case -1 : strerr_diefu1sys(111, "complete exlsn function") ;
    default : strerr_diefu2x(111, "complete exlsn function", ": unknown error") ;
  }
  el_substandrun(argc-r, argv+r, envp, &info) ;
}
