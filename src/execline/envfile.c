/* ISC license. */

#include <stdint.h>
#include <string.h>

#include <skalibs/types.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#define USAGE "envfile file prog..."
#define dieusage() strerr_dieusage(100, USAGE)
#define dienomem() strerr_diefu1sys(111, "stralloc_catb")

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
    default : return 5 ;
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

static inline void parse_config (char const *file, buffer *b, stralloc *modif)
{
  static uint8_t const table[7][6] =
  {
    { 0x07, 0x01, 0x80, 0x08, 0x00, 0x22 },
    { 0x07, 0x01, 0x80, 0x01, 0x01, 0x01 },
    { 0x08, 0x08, 0x08, 0x24, 0x03, 0x22 },
    { 0x08, 0x08, 0x08, 0x24, 0x03, 0x08 },
    { 0x47, 0x41, 0xc0, 0x25, 0x04, 0x25 },
    { 0x47, 0x41, 0xc0, 0x25, 0x46, 0x25 },
    { 0x07, 0x01, 0x80, 0x08, 0x06, 0x08 }
  } ;
  unsigned int line = 1 ;
  uint8_t state = 0 ;
  while (state < 7)
  {
    char c = next(file, b) ;
    uint8_t what = table[state][cclass(c)] ;
    state = what & 0x0f ;
    if (what & 0x20)
      if (!stralloc_catb(modif, &c, 1)) dienomem() ;
    if (what & 0x40)
      if (!stralloc_0(modif)) dienomem() ;
    if (what & 0x80) line++ ;
  }
  if (state > 7)
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, line)] = 0 ;
    strerr_dief4x(1, "syntax error line ", fmt, " while parsing ", file) ;
  }
}

int main (int argc, char const *const *argv, char const *const *envp)
{
  stralloc modif = STRALLOC_ZERO ;
  PROG = "envfile" ;
  if (argc < 3) dieusage() ;
  if (!strcmp(argv[1], "-"))
    parse_config("standard input", buffer_0, &modif) ;
  else
  {
    buffer b ;
    char buf[BUFFER_INSIZE] ;
    int fd = open_readb(argv[1]) ;
    if (fd == -1) strerr_diefu2sys(111, "open ", argv[1]) ;
    buffer_init(&b, &buffer_read, fd, buf, BUFFER_INSIZE) ;
    parse_config(argv[1], &b, &modif) ;
    fd_close(fd) ;
  }
  xpathexec_r(argv + 2, envp, env_len(envp), modif.s, modif.len) ;
}
