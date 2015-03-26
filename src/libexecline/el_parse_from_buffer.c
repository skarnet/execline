/* ISC license. */

#include <skalibs/buffer.h>
#include <skalibs/stralloc.h>
#include <execline/execline.h>

static int next (unsigned char *c, void *p)
{
  register int r = buffer_get((buffer *)p, (char *)c, 1) ;
  if (r < 0) return 0 ;
  if (!r) *c = 0 ;
  return 1 ;
}

int el_parse_from_buffer (stralloc *sa, buffer *b)
{
  return el_parse(sa, &next, b) ;
}
