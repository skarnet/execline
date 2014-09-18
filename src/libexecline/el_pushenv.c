/* ISC license. */

#include <errno.h>
#include <skalibs/bytestr.h>
#include <skalibs/stralloc.h>
#include <skalibs/uint.h>
#include <execline/execline.h>

int el_pushenv (stralloc *sa, char const *const *envp, unsigned int envlen, char const *const *list, unsigned int listlen)
{
  unsigned int i = 0, salen = sa->len, count = 0 ;
  for (; i < envlen ; i++)
  {
    unsigned int equal, colon ;
    unsigned int j = 0 ;
    for (; j < listlen ; j++) if (str_start(envp[i], list[j])) break ;
    if (j == listlen) goto copyit ;
    count++ ;
    j = str_len(list[j]) ;
    colon = j + str_chr(envp[i] + j, ':') ;
    equal = j + str_chr(envp[i] + j, '=') ;
    if (!envp[i][equal]) goto badenv ;
    if (colon >= equal)
    {
      if (!stralloc_catb(sa, envp[i], equal)
       || !stralloc_catb(sa, ":1", 2)) goto err ;
    }
    else
    {
      char fmt[UINT_FMT+1] = ":" ;
      unsigned int n ;
      if (colon + 1 + uint_scan(envp[i] + colon + 1, &n) != equal) goto copyit ;
      n = 1 + uint_fmt(fmt+1, n+1) ;
      if (!stralloc_catb(sa, envp[i], colon)) goto err ;
      if (!stralloc_catb(sa, fmt, n)) goto err ;
    }
    if (!stralloc_catb(sa, envp[i] + equal, str_len(envp[i] + equal) + 1)) goto err ;
    continue ;
copyit:
    if (!stralloc_catb(sa, envp[i], str_len(envp[i]) + 1)) goto err ;
  }
  return (int)count ;

badenv :
  errno = EINVAL ;
err:
  sa->len = salen ;
  return -1 ;
}
