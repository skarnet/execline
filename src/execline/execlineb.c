/* ISC license. */

#include <skalibs/uint16.h>
#include <skalibs/uint.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <execline/execline.h>
#include "exlsn.h"

#define USAGE "execlineb [ -p | -P | -S nmin ] [ -q | -w | -W ] [ -c commandline ] script args"

typedef unsigned char chargen_t (void) ;

/* Action (strongest 11 bits) */

#define PUSH 0x8000
#define PUSH0 0x4000
#define PUSHSPECIAL 0x2000
#define SETBASE 0x1000
#define MARK 0x0800
#define CALC 0x0400
#define QUOTE 0x0200
#define INCB 0x0100
#define DECB 0x0080


/* State (weakest 5 bits) */

#define MAIN 0x00
#define INWORD 0x01
#define INWORDESC 0x02
#define INSTR 0x03
#define INSTRESC 0x04
#define INREM 0x05
#define OCT0 0x06
#define OCT1 0x07
#define OCT2 0x08
#define DEC1 0x09
#define DEC2 0x0a
#define HEX0 0x0b
#define HEX1 0x0c
#define ENDCALC 0x0d
#define OPENB 0x0e
#define CLOSEB 0x0f
#define ERROR 0x10
#define ACCEPT 0x11

static buffer b ;

static void initbuffer (char const *s)
{
  static char buf[BUFFER_INSIZE] ;
  int fd = open_readb(s) ;
  if (fd < 0) strerr_diefu3sys(111, "open ", s, " for reading") ;
  if (coe(fd) < 0) strerr_diefu2sys(111, "coe ", s) ;
  buffer_init(&b, &buffer_read, fd, buf, BUFFER_INSIZE) ;
}

static unsigned char nextinbuffer ()
{
  char c ;
  switch (buffer_get(&b, &c, 1))
  {
    case -1: strerr_diefu1sys(111, "read script") ;
    case 0 : return 0 ;
  }
  return (unsigned char)c ;
}

static unsigned char const *string = 0 ;

static unsigned char nextinstring ()
{
  static unsigned int pos = 0 ;
  return string[pos++] ;
}

static int lex (stralloc *sa, chargen_t *next)
{
  static unsigned char const class[256] = "`aaaaaaaaadaaaaaaaaaaaaaaaaaaaaaafcbffffffffffffjhhhhhhhiifffffffmmmmmmfffffffffffffffffffffeffffggmmmgfffffffkfffkfkfkflffnfoffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" ;
  static uint16 const table[16][16] =
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

  unsigned int mark = 0 ;
  unsigned int n = 0 ;
  unsigned char state = MAIN, base = 10 ;
  unsigned int blevel = 0 ;

  while (state < ERROR)
  {
    unsigned char cur = (*next)() ;
    register uint16 c = table[class[cur]-'`'][state] ;
    state = c & 0x1F ;

 /* Actions. The order is important ! */

    if (c & CALC)
    {
      unsigned int z ;
      if (!stralloc_0(sa)) return -1 ;
      sa->len = mark ;
      uint_scan_base(sa->s + sa->len, &z, base) ;
      sa->s[sa->len++] = (unsigned char)z ;
    }
    if (c & MARK) mark = sa->len ;
    if (c & QUOTE)
    {
      char tilde = EXECLINE_BLOCK_QUOTE_CHAR ;
      register unsigned int i = blevel ;
      if (!stralloc_readyplus(sa, i<<1)) return -1 ;
      while (i--) stralloc_catb(sa, &tilde, 1) ;
    }
    if (c & INCB) sa->len -= ++blevel ;
    if (c & DECB)
    {
      if (!blevel--) return -4 ;
      sa->s[--sa->len-1] = EXECLINE_BLOCK_END_CHAR ;
      if (!EXECLINE_BLOCK_END_CHAR) sa->len-- ;
    }
    if (c & PUSH) if (!stralloc_catb(sa, (char *)&cur, 1)) return -1 ;
    if (c & PUSHSPECIAL)
    {
      char x = 7 + byte_chr("abtnvfr", 7, cur) ;
      if (!stralloc_catb(sa, &x, 1)) return -1 ;
    }
    if (c & PUSH0) if (n++, !stralloc_0(sa)) return -1 ;
    if (c & SETBASE)
      switch (cur)
      {
        case 'x' : base = 16 ; break ;
        case '0' : base = 8 ; break ;
        default : base = 10 ;
      }
  }
  if (state == ERROR) return -2 ;
  if (blevel) return -3 ;
  return n ;
}


static int myexlp (stralloc *sa, char const *const *argv, unsigned int argc, unsigned int nmin, char const *dollar0)
{
  exlsn_t info = EXLSN_ZERO ;
  unsigned int n = argc > nmin ? argc : nmin ;
  unsigned int i = 0 ;

  if (!genalloc_ready(elsubst_t, &info.data, 3 + n)) return -1 ;
  if (!stralloc_ready(&info.vars, 6 + (n << 1))) goto err ;
  stralloc_catb(&info.vars, "#\0" "0\0@", 6) ;
  {
    elsubst_t blah[3] ;
    char fmt[UINT_FMT] ;
    blah[0].var = 0 ; blah[0].value = 0 ; blah[0].n = 1 ;
    if (!stralloc_catb(&info.values, fmt, uint_fmt(fmt, argc)) || !stralloc_0(&info.values)) goto err ;
    blah[1].var = 2 ; blah[1].value = info.values.len ; blah[1].n = 1 ;
    if (!stralloc_catb(&info.values, dollar0, str_len(dollar0) + 1)) goto err ;
    blah[2].var = 4 ; blah[2].value = info.values.len ; blah[2].n = argc ;
    genalloc_catb(elsubst_t, &info.data, blah, 3) ;
  }
  for (; i < n ; i++)
  {
    elsubst_t blah ;
    char fmt[UINT_FMT] ;
    blah.var = info.vars.len ; blah.value = info.values.len ; blah.n = 1 ;
    if (!stralloc_catb(&info.vars, fmt, uint_fmt(fmt, i+1)) || !stralloc_0(&info.vars)) goto err ;
    if (!stralloc_catb(&info.values, i < argc ? argv[i] : "", i < argc ? str_len(argv[i]) + 1 : 1)) goto err ;
    genalloc_append(elsubst_t, &info.data, &blah) ;
  }
  {
    stralloc dst = STRALLOC_ZERO ;
    int r = el_substitute(&dst, sa->s, sa->len, info.vars.s, info.values.s, genalloc_s(elsubst_t, &info.data), genalloc_len(elsubst_t, &info.data)) ;
    if (r < 0) goto err ;
    exlsn_free(&info) ;
    stralloc_free(sa) ;
    *sa = dst ;
    return r ;
  }

 err:
  exlsn_free(&info) ;
  return -1 ;
}


int main (int argc, char const *const *argv, char const *const *envp)
{
  chargen_t *next ;
  stralloc sa = STRALLOC_ZERO ;
  stralloc modif = STRALLOC_ZERO ;
  int nc ;
  int flagstrict = -1 ;
  unsigned int nmin = 0 ;
  char const *dollar0 = argv[0] ;
  unsigned int flagpushenv = 2 ;
  PROG = "execlineb" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "pPqwWc:S:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' : flagpushenv = 1 ; break ;
        case 'P' : flagpushenv = 0 ; break ;
        case 'q' : flagstrict = 0 ; break ;
        case 'w' : flagstrict = 1 ; break ;
        case 'W' : flagstrict = 2 ; break ;
        case 'c' : string = (unsigned char *)l.arg ; break ;
        case 'S' :
        {
          if (!uint0_scan(l.arg, &nmin)) strerr_dieusage(100, USAGE) ;
          flagpushenv = 3 ;
          break ;
        }
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (string) next = &nextinstring ;
  else
  {
    if (!argv[0]) strerr_dieusage(100, USAGE) ;
    initbuffer(argv[0]) ;
    dollar0 = argv[0] ;
    argv++ ; argc-- ;
    next = &nextinbuffer ;
  }

  nc = lex(&sa, next) ;
  switch (nc)
  {
    case -4: strerr_dief2x(100, "unmatched ", "}") ;
    case -3: strerr_dief2x(100, "unmatched ", "{") ;
    case -2: strerr_dief1x(100, "syntax error") ;
    case -1: strerr_diefu1sys(111, "parse script") ;
    case 0 : return 0 ;
  }

  if (flagstrict >= 0)
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, (unsigned int)flagstrict)] = 0 ;
    if (!env_addmodif(&modif, "EXECLINE_STRICT", flagstrict ? fmt : 0)) goto errenv ;
  }

  if (flagpushenv == 3)
  {
    flagpushenv = 0 ;
    if (flagstrict && ((unsigned int)argc < nmin))
    {
      char fmtn[UINT_FMT] ;
      char fmta[UINT_FMT] ;
      fmtn[uint_fmt(fmtn, nmin)] = 0 ;
      fmta[uint_fmt(fmta, argc)] = 0 ;
      if (flagstrict > 1)
        strerr_dief4x(100, "too few arguments: expecting at least ", fmtn, " but got ", fmta) ;
      else
        strerr_warnw4x("too few arguments: expecting at least ", fmtn, " but got ", fmta) ;
    }
    nc = myexlp(&sa, argv, argc, nmin, dollar0) ;
    if (nc < 0) strerr_diefu1sys(111, "substitute positional parameters") ;
  }
  else if (flagpushenv)
  {
    char fmt[UINT_FMT] ;
    register unsigned int i = 0 ;
    fmt[uint_fmt(fmt, argc)] = 0 ;
    if (!env_addmodif(&modif, "#", fmt)) goto errenv ;
    if (!env_addmodif(&modif, "0", dollar0)) goto errenv ;
    for (; i < (unsigned int)argc ; i++)
    {
      fmt[uint_fmt(fmt, i+1)] = 0 ;
      if (!env_addmodif(&modif, fmt, argv[i])) goto errenv ;
    }
  }

  {
    char const *v[nc+1] ;
    if (!env_make(v, nc, sa.s, sa.len)) strerr_diefu1sys(111, "make argv") ;
    v[nc] = 0 ;

    if (flagpushenv > 1)
    {
      static char const *const list[11] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "#" } ;
      unsigned int envlen = env_len(envp) ;
      char const *w[envlen] ;
      if (el_pushenv(&satmp, envp, envlen, list, 11) < 0) goto errenv ;
      if (!env_make(w, envlen, satmp.s, satmp.len)) goto errenv ;
      pathexec_r(v, w, envlen, modif.s, modif.len) ;
      stralloc_free(&satmp) ;
    }
    else if (modif.len)
      pathexec_r(v, envp, env_len(envp), modif.s, modif.len) ;
    else
      pathexec_run(v[0], v, envp) ;
  }
  stralloc_free(&modif) ;
  strerr_dieexec(111, sa.s) ;
errenv:
  strerr_diefu1sys(111, "update environment") ;  
}
