/* ISC license. */

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <skalibs/bytestr.h>
#include <skalibs/types.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/sgetopt.h>
#include <skalibs/fmtscan.h>
#include <skalibs/env.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "envfile [ -i | -I ] file prog..."
#define dieusage() strerr_dieusage(100, USAGE)
#define dienomem() strerr_diefu1sys(111, "stralloc_catb")

static void scanoct (stralloc *sa, size_t pos)
{
  unsigned int u ;
  if (!stralloc_0(sa)) dienomem() ;
  uint_oscan(sa->s + pos, &u) ;
  sa->s[pos] = u ;
  sa->len = pos+1 ;
}

static inline uint8_t cclass (char c)
{
  switch (c)
  {
    case 0 : return 0 ;
    case '#' : return 1 ;
    case '\n' : return 2 ;
    case '=' : return 3 ;
    case ' ' :
    case '\t' :
    case '\f' :
    case '\r' : return 4 ;
    case '\\' : return 5 ;
    case '\"' : return 6 ;
    case 'a' :
    case 'b' :
    case 'f' : return 7 ;
    case 'n' :
    case 'r' :
    case 't' :
    case 'v' : return 8 ;
    case '\'' :
    case '?' : return 9 ;
    case 'x' : return 10 ;
    case '0' :
    case '1' :
    case '2' :
    case '3' :
    case '4' :
    case '5' :
    case '6' :
    case '7' : return 11 ;
    case '8' :
    case '9' :
    case 'A' :
    case 'B' :
    case 'c' :
    case 'C' :
    case 'd' :
    case 'D' :
    case 'e' :
    case 'E' :
    case 'F' : return 12 ;
    default : return 13 ;
  }
}

static inline char next (char const *file, buffer *b)
{
  char c ;
  ssize_t r = buffer_get(b, &c, 1) ;
  if (r < 0) strerr_diefu2sys(111, "read from ", file) ;
  if (!r) c = 0 ;
  return c ;
}

static inline void parse_config (char const *file, buffer *b, stralloc *sa)
{
  static uint16_t const table[14][14] =
  {
    { 0x000e, 0x0001, 0x0020, 0x000f, 0x0000, 0x000f, 0x000f, 0x0012, 0x0012, 0x000f, 0x0012, 0x0012, 0x0012, 0x0012 },
    { 0x000e, 0x0001, 0x0020, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
    { 0x000f, 0x0012, 0x000f, 0x0014, 0x0003, 0x000f, 0x000f, 0x0012, 0x0012, 0x000f, 0x0012, 0x0012, 0x0012, 0x0012 },
    { 0x000f, 0x000f, 0x000f, 0x0014, 0x0003, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f },
    { 0x004e, 0x0015, 0x0060, 0x0015, 0x0004, 0x0008, 0x0007, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015 },
    { 0x004e, 0x0015, 0x0060, 0x0015, 0x0116, 0x0008, 0x0007, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015 },
    { 0x00ce, 0x0015, 0x00d0, 0x0015, 0x0016, 0x0008, 0x0007, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015 },
    { 0x000f, 0x0017, 0x000f, 0x0017, 0x0017, 0x0009, 0x0005, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017 },
    { 0x000f, 0x000f, 0x0025, 0x000f, 0x0015, 0x0015, 0x0015, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f },
    { 0x000f, 0x0017, 0x0027, 0x0017, 0x0017, 0x0017, 0x0017, 0x1017, 0x1017, 0x0017, 0x000a, 0x011c, 0x0017, 0x0017 },
    { 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x001b, 0x000f, 0x000f, 0x000f, 0x001b, 0x001b, 0x000f },
    { 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x0217, 0x000f, 0x000f, 0x000f, 0x0217, 0x0217, 0x000f },
    { 0x000f, 0x0417, 0x000f, 0x0417, 0x0417, 0x0409, 0x0405, 0x0417, 0x0417, 0x0417, 0x0417, 0x001d, 0x0417, 0x0417 },
    { 0x000f, 0x0417, 0x000f, 0x0417, 0x0417, 0x0409, 0x0405, 0x0417, 0x0417, 0x0417, 0x0417, 0x0817, 0x0417, 0x0417 }
  } ;
  unsigned int line = 1 ;
  size_t mark = 0 ;
  uint8_t state = 0 ;
  while (state < 14)
  {
    char c = next(file, b) ;
    uint16_t what = table[state][cclass(c)] ;
    state = what & 0x0f ;
    if (what & 0x0400) scanoct(sa, mark) ;
    if (what & 0x0100) mark = sa->len ;
    if (what & 0x1000) c = 7 + byte_chr("abtnvfr", 7, c) ;
    if (what & 0x0010) if (!stralloc_catb(sa, &c, 1)) dienomem() ;
    if (what & 0x0020) line++ ;
    if (what & 0x0080) sa->len = mark ;
    if (what & 0x0040) if (!stralloc_0(sa)) dienomem() ;
    if (what & 0x0200)
    {
      sa->s[sa->len-2] = (fmtscan_num(sa->s[sa->len-2], 16) << 4) + fmtscan_num(sa->s[sa->len-1], 16) ;
      sa->len-- ;
    }
    if (what & 0x0800) scanoct(sa, mark) ;
  }
  if (state > 14)
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, line)] = 0 ;
    strerr_dief4x(1, "in ", file, ": syntax error line ", fmt) ;
  }
}

int main (int argc, char const *const *argv)
{
  stralloc modif = STRALLOC_ZERO ;
  int fd ;
  char const *name ;
  int strict = 1 ;
  PROG = "envfile" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "iI", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'i' : strict = 1 ; break ;
        case 'I' : strict = 0 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  if (argc < 2) dieusage() ;
  if (strcmp(argv[0], "-"))
  {
    fd = open_readb(argv[0]) ;
    name = argv[0] ;
  }
  else
  {
    fd = 0 ;
    name = "standard input" ;
  }
  if (fd == -1)
  {
    if (strict || errno != ENOENT)
      strerr_diefu2sys(111, "open ", name) ;
  }
  else
  {
    buffer b ;
    char buf[BUFFER_INSIZE] ;
    buffer_init(&b, &buffer_read, fd, buf, BUFFER_INSIZE) ;
    parse_config(name, &b, &modif) ;
    fd_close(fd) ;
  }
  xmexec_m(argv + 1, modif.s, modif.len) ;
}
