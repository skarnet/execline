/* ISC license. */

#include <sys/types.h>
#include <errno.h>
#include <skalibs/bytestr.h>
#include <skalibs/stralloc.h>
#include <skalibs/uint.h>
#include <execline/execline.h>

int el_popenv (stralloc *sa, char const *const *envp, size_t envlen, char const *const *list, size_t listlen)
{
  size_t i = 0, salen = sa->len ;
  int count = 0 ;
  for (; i < envlen ; i++)
  {
    size_t equal, colon, n, j = 0 ;
    for (; j < listlen ; j++) if (str_start(envp[i], list[j])) break ;
    if (j == listlen) goto copyit ;
    j = str_len(list[j]) ;
    colon = j + str_chr(envp[i] + j, ':') ;
    equal = j + str_chr(envp[i] + j, '=') ;
    if (!envp[i][equal]) goto badenv ;
    if (colon >= equal) { count++ ; continue ; }
    if (colon + 1 + uint_scan(envp[i] + colon + 1, &n) != equal) goto copyit ;
    if (!n) goto copyit ;
    if (!stralloc_catb(sa, envp[i], colon)) goto err ;
    if (n > 1)
    {
      char fmt[UINT_FMT+1] = ":" ;
      n = 1 + uint_fmt(fmt+1, n-1) ;
      if (!stralloc_catb(sa, fmt, n)) goto err ;
    }
    if (!stralloc_catb(sa, envp[i] + equal, str_len(envp[i] + equal) + 1)) goto err ;
    continue ;
copyit:
    if (!stralloc_catb(sa, envp[i], str_len(envp[i]) + 1)) goto err ;
  }
  return count ;

badenv :
  errno = EINVAL ;
err:
  sa->len = salen ;
  return -1 ;
}
