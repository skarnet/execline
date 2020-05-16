/* ISC license. */

#include <string.h>
#include <skalibs/bytestr.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <execline/execline.h>

typedef struct elsubsu_s elsubsu_t, *elsubsu_t_ref ;
struct elsubsu_s
{
  elsubst_t const *subst ;
  size_t pos ;
} ;

typedef struct subsuinfo_s subsuinfo_t, *subsuinfo_t_ref ;
struct subsuinfo_s
{
  stralloc dst ;
  stralloc sa ;
  genalloc list ; /* array of elsubsu_t */
  char const *values ;
} ;

#define SUBSUINFO_ZERO { .dst = STRALLOC_ZERO, .sa = STRALLOC_ZERO, .list = GENALLOC_ZERO, .values = 0 }

#define TEST 0x80
#define MARK 0x40
#define KEEPESC 0x20
#define INCRESC 0x10

#define STATE 0x07
#define INWORD 0x00
#define INDOLL 0x01
#define INDBR 0x02
#define INVAR 0x03
#define INVARBR 0x04
#define ACCEPT 0x05

static ssize_t parseword (stralloc *sa, genalloc *list, char const *s, char const *vars, elsubst_t const *substs, unsigned int nsubst)
{
  static char const class[5] = "\0\\${}" ;
  static unsigned char const table[6][5] =
  {
    { ACCEPT, ACCEPT, ACCEPT, ACCEPT | TEST, ACCEPT },
    { INWORD | KEEPESC | INCRESC, INWORD | INCRESC, INWORD | INCRESC, INWORD | TEST | INCRESC, INWORD | INCRESC },
    { INDOLL | KEEPESC, INDOLL, INDOLL, INDOLL | TEST, INDOLL },
    { INWORD, INDBR | KEEPESC, INWORD, INWORD | TEST, INWORD },
    { INWORD, INWORD, INWORD, INWORD | TEST, INWORD | TEST },
    { INWORD, INVAR | MARK | KEEPESC, INVARBR | MARK | KEEPESC, INVAR | KEEPESC, INVARBR | KEEPESC }
  } ;

  size_t mark = 0, offset = 0, esc = 0, salen = sa->len, listlen = genalloc_len(elsubsu_t, list) ;
  ssize_t pos = 0 ;
  unsigned char state = INWORD ;

  while (state != ACCEPT)
  {
    int nopush = 0 ;
    unsigned char c = table[byte_chr(class, 5, s[pos])][state] ;

    if (c & TEST)
    {
      unsigned int supp = (state == INVARBR) ;
      unsigned int i = 0 ;
      for (; i < nsubst ; i++)
      {
        if (!strncmp(vars + substs[i].var, s + mark, pos - mark) && !vars[substs[i].var + pos - mark])
        {
          sa->len -= esc >> 1 ; offset += esc >> 1 ;
          if (esc & 1)
          {
            memmove(sa->s + mark - offset - 2 - supp, sa->s + mark - offset + (esc>>1) - 1 - supp, pos - mark + 1 + supp) ;
            sa->len-- ; offset++ ;
          }
          else
          {
            elsubsu_t cur ;
            cur.subst = substs + i ;
            cur.pos = mark - offset - 1 - supp ;
            if (!genalloc_append(elsubsu_t, list, &cur)) goto err ;
            offset += sa->len - cur.pos ;
            sa->len = cur.pos ;
            if (supp) nopush = 1 ;
          }
          break ;
        }
      }
    }
    if (nopush) offset++ ;
    else if (!stralloc_catb(sa, s+pos, 1)) goto err ;
    if (c & MARK) mark = pos ;
    if (!(c & KEEPESC)) esc = 0 ;
    if (c & INCRESC) esc++ ;
    state = c & STATE ; pos++ ;
  }
  sa->len-- ;
  return pos ;

err:
  sa->len = salen ;
  list->len = listlen ;
  return -1 ;
}

static int substword (subsuinfo_t *info, size_t wordstart, size_t wordlen, unsigned int n, size_t offset)
{
  if (n < genalloc_len(elsubsu_t, &info->list))
  {
    elsubsu_t *list = genalloc_s(elsubsu_t, &info->list) ;
    char const *p = info->values + list[n].subst->value ;
    size_t l = list[n].pos + offset ;
    size_t dstbase = info->dst.len ;
    size_t sabase = info->sa.len ;
    unsigned int i = 0 ;
    int nc = 0 ;
    if (!stralloc_readyplus(&info->sa, l)) return -1 ;
    stralloc_catb(&info->sa, info->sa.s + wordstart, l) ;
    for (; i < list[n].subst->n ; i++)
    {
      size_t plen = strlen(p) ;
      int r ;
      info->sa.len = sabase + l ;
      if (!stralloc_readyplus(&info->sa, plen + wordlen - l)) goto err ;
      stralloc_catb(&info->sa, p, plen) ;
      stralloc_catb(&info->sa, info->sa.s + wordstart + l, wordlen - l) ;
      r = substword(info, sabase, info->sa.len - sabase, n+1, offset + plen) ;
      if (r < 0) goto err ;
      nc += r ;
      p += plen+1 ;
    }
    return nc ;
   err:
    info->sa.len = sabase ;
    info->dst.len = dstbase ;
    return -1 ;
  }
  else
  {
    if (!stralloc_readyplus(&info->dst, wordlen+1)) return -1 ;
    stralloc_catb(&info->dst, info->sa.s + wordstart, wordlen) ;
    stralloc_0(&info->dst) ;
    return 1 ;
  }
}

int el_substitute (stralloc *dst, char const *src, size_t len, char const *vars, char const *values, elsubst_t const *substs, unsigned int nsubst)
{
  subsuinfo_t info = SUBSUINFO_ZERO ;
  size_t i = 0 ;
  int nc = 0 ;
  int wasnull = !dst->s ;

  if (!stralloc_copy(&info.dst, dst)) return -1 ;
  info.values = values ;

  while (i < len)
  {
    genalloc_setlen(elsubsu_t, &info.list, 0) ;
    info.sa.len = 0 ;
    {
      ssize_t r = parseword(&info.sa, &info.list, src + i, vars, substs, nsubst) ;
      if (r < 0) goto err ;
      i += r ;
    }
    {
      int r = substword(&info, 0, info.sa.len, 0, 0) ;
      if (r < 0) goto err ;
      nc += r ;
    }
  }
  genalloc_free(elsubsu_t, &info.list) ;
  stralloc_free(&info.sa) ;
  if (!wasnull) stralloc_free(dst) ;
  *dst = info.dst ;
  return nc ;

err :
  genalloc_free(elsubsu_t, &info.list) ;
  stralloc_free(&info.sa) ;
  stralloc_free(&info.dst) ;
  return -1 ;
}
