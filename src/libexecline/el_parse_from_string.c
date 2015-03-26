/* ISC license. */

#include <skalibs/stralloc.h>
#include <execline/execline.h>

static int next (unsigned char *c, void *p)
{
  *c = *(*(char const **)p)++ ;
  return 1 ;
}

int el_parse_from_string (stralloc *sa, char const *s)
{
  return el_parse(sa, &next, &s) ;
}
