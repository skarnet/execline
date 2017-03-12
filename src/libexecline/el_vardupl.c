/* ISC license. */

#include <string.h>
#include <execline/execline.h>

int el_vardupl (char const *key, char const *s, size_t len)
{
  size_t i = 0 ;
  for (; i < len ; i += strlen(s + i) + 1)
    if (!strcmp(key, s + i)) return 1 ;
  return 0 ;
}
