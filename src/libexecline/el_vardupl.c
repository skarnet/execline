/* ISC license. */

#include <sys/types.h>
#include <skalibs/bytestr.h>
#include <execline/execline.h>

int el_vardupl (char const *key, char const *s, size_t len)
{
  register size_t i = 0 ;
  for (; i < len ; i += str_len(s + i) + 1)
    if (!str_diff(key, s + i)) return 1 ;
  return 0 ;
}
