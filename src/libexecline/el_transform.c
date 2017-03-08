/* ISC license. */

#include <sys/types.h>
#include <skalibs/bytestr.h>
#include <skalibs/netstring.h>
#include <skalibs/skamisc.h>
#include <skalibs/stralloc.h>
#include <execline/execline.h>

static void el_crunch (stralloc *sa, size_t base, char const *delim)
{
  size_t i = base, j = base ;
  int crunching = 0 ;
  for (; i < sa->len ; i++)
  {
    if (!crunching) sa->s[j++] = sa->s[i] ;
    if (delim[str_chr(delim, sa->s[i])]) crunching = 1 ;
    else if (crunching)
    {
      i-- ;
      crunching = 0 ;
    }
  }
  sa->len = j ;
}

static int el_split (stralloc *sa, size_t base, eltransforminfo_t const *si, int chomped)
{
  int n = 0 ;
  size_t i = base ;
  for (; i < sa->len ; i++)
    if (si->delim[str_chr(si->delim, sa->s[i])])
    {
      sa->s[i] = 0 ;
      n++ ;
      base = i+1 ;
    }

  if (sa->len && sa->s[sa->len - 1])
  {
    if (si->chomp && !chomped) sa->len = base ;
    else if (!stralloc_0(sa)) return -1 ;
    else n++ ;
  }
  return n ;
}

static int el_splitnetstring (stralloc *sa, size_t base)
{
  size_t tmpbase = satmp.len, i = base ;
  int n = 0 ;
  while (i < sa->len)
  {
    ssize_t r = netstring_decode(&satmp, sa->s + i, sa->len - i) ;
    if (r < 0) goto err ;
    if (!stralloc_0(&satmp)) goto err ;
    i += r ; n++ ;
  }
  sa->len = base ;
  if (!stralloc_catb(sa, satmp.s + tmpbase, satmp.len - tmpbase))
  {
    sa->len = i ;
    goto err ;
  }
  satmp.len = tmpbase ;
  return n ;

err:
  satmp.len = tmpbase ;
  return -1 ;
}

int el_transform (stralloc *sa, size_t i, eltransforminfo_t const *si)
{
  int chomped = 0 ;
  if (si->crunch && *si->delim) el_crunch(sa, i, si->delim) ;
  if (si->chomp && (sa->len > i)
   && si->delim[str_chr(si->delim, sa->s[sa->len-1])])
  {
    sa->len-- ;
    chomped = 1 ;
  }
  return si->split ? *si->delim ? el_split(sa, i, si, chomped) : el_splitnetstring(sa, i) :
                     stralloc_0(sa) ? 1 : -1 ;
}
