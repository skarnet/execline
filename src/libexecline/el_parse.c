/* ISC license. */

#include <sys/types.h>
#include <stdint.h>
#include <skalibs/types.h>
#include <skalibs/bytestr.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

int el_parse (stralloc *sa, el_chargen_func_ref next, void *source)
{
  static unsigned char const class[256] = "`aaaaaaaaadaaaaaaaaaaaaaaaaaaaaaafcbffffffffffffjhhhhhhhiifffffffmmmmmmfffffffffffffffffffffeffffggmmmgfffffffkfffkfkfkflffnfoffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" ;
  static uint16_t const table[16][16] =
  {
    { 0x0011, 0x4011, 0x0010, 0x0010, 0x0010, 0x0011, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x4091 },
    { 0x0000, 0x4000, 0x8001, 0x8003, 0x8003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x0100, 0x4080 },
    { 0x0005, 0x8001, 0x8001, 0x8003, 0x8003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x8001, 0x8001 },
    { 0x0203, 0x0003, 0x8001, 0x0001, 0x8003, 0x0005, 0x0010, 0x0401, 0x0401, 0x0401, 0x0401, 0x0010, 0x0401, 0x0401, 0x0003, 0x0003 },
    { 0x0000, 0x4000, 0x8001, 0x8003, 0x0003, 0x0000, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x0100, 0x4080 },
    { 0x0202, 0x0002, 0x8001, 0x0004, 0x8003, 0x0005, 0x0010, 0x0404, 0x0404, 0x0404, 0x0404, 0x0010, 0x0404, 0x0404, 0x0002, 0x0002 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x8003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x8001, 0x8001 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x2003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x880c, 0x800d, 0x8403, 0x8001, 0x8001 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x9809, 0x0005, 0x8807, 0x8008, 0x800d, 0x800a, 0x800d, 0x880c, 0x800d, 0x8403, 0x8001, 0x8001 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x9809, 0x0005, 0x0010, 0x8403, 0x8403, 0x800a, 0x800d, 0x880c, 0x800d, 0x8403, 0x8001, 0x8001 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x1006, 0x0005, 0x8807, 0x8008, 0x800d, 0x800a, 0x800d, 0x880c, 0x800d, 0x8403, 0x8001, 0x8001 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x2003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x8001, 0x8001 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x8003, 0x0005, 0x100b, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x8001, 0x8001 },
    { 0x8201, 0x8001, 0x8001, 0x8003, 0x8003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x880c, 0x800d, 0x8403, 0x8001, 0x8001 },
    { 0x820e, 0x8001, 0x8001, 0x8003, 0x8003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x8001, 0x8001 },
    { 0x820f, 0x8001, 0x8001, 0x8003, 0x8003, 0x0005, 0x0010, 0x8403, 0x8403, 0x8403, 0x8403, 0x0010, 0x8403, 0x8403, 0x8001, 0x8001 }
  } ;

  size_t mark = 0 ;
  int n = 0 ;
  unsigned int blevel = 0 ;
  unsigned char state = 0, base = 10 ;

  while (state < 0x10)
  {
    uint16_t c ;
    unsigned char cur ;
    if (!(*next)(&cur, source)) return -1 ;
    c = table[class[cur]-'`'][state] ;
    state = c & 0x1F ;

    if (c & 0x0400)
    {
      unsigned int z ;
      if (!stralloc_0(sa)) return -1 ;
      sa->len = mark ;
      uint_scan_base(sa->s + sa->len, &z, base) ;
      sa->s[sa->len++] = (unsigned char)z ;
    }
    if (c & 0x0800) mark = sa->len ;
    if (c & 0x0200)
    {
      char tilde = EXECLINE_BLOCK_QUOTE_CHAR ;
      unsigned int i = blevel ;
      if (!stralloc_readyplus(sa, i<<1)) return -1 ;
      while (i--) stralloc_catb(sa, &tilde, 1) ;
    }
    if (c & 0x0100) sa->len -= ++blevel ;
    if (c & 0x0080)
    {
      if (!blevel--) return -4 ;
      sa->s[--sa->len-1] = EXECLINE_BLOCK_END_CHAR ;
      if (!EXECLINE_BLOCK_END_CHAR) sa->len-- ;
    }
    if (c & 0x8000) if (!stralloc_catb(sa, (char *)&cur, 1)) return -1 ;
    if (c & 0x2000)
    {
      char x = 7 + byte_chr("abtnvfr", 7, cur) ;
      if (!stralloc_catb(sa, &x, 1)) return -1 ;
    }
    if (c & 0x4000) if (n++, !stralloc_0(sa)) return -1 ;
    if (c & 0x1000)
      switch (cur)
      {
        case 'x' : base = 16 ; break ;
        case '0' : base = 8 ; break ;
        default : base = 10 ;
      }
  }
  if (state == 0x10) return -2 ;
  if (blevel) return -3 ;
  return n ;
}
