/* ISC license. */

#include <skalibs/stralloc.h>
#include "exlsn.h"

void exlsn_free (exlsn_t *info)
{
  stralloc_free(&info->vars) ;
  stralloc_free(&info->values) ;
  stralloc_free(&info->data) ;
  stralloc_free(&info->modifs) ;
}

