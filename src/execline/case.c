/* ISC license. */

#include <string.h>
#include <regex.h>
#include <fnmatch.h>

#include <skalibs/uint64.h>
#include <skalibs/gccattributes.h>
#include <skalibs/types.h>
#include <skalibs/envexec.h>

#include <execline/execline.h>

#define USAGE "case [ -s | -S ] [ -e | -E ] [ -n | -N ] [ -I | -i ] value { re1 { prog1... } re2 { prog2... } ... } progdefault... "
#define dieusage() strerr_dieusage(100, USAGE)

enum golb_e
{
  GOLB_SHELL = 0x01,
  GOLB_BRE = 0x02,
  GOLB_SUB = 0x04,
  GOLB_ICASE = 0x08,
} ;

static void case_execit (char const *const *argv, char const *expr, char const *s, regmatch_t const *pmatch, size_t n) gccattr_noreturn ;
static void case_execit (char const *const *argv, char const *expr, char const *s, regmatch_t const *pmatch, size_t n)
{
  if (n)
  {
    size_t exprlen = strlen(expr) ;
    size_t fmtlen = exprlen + 6 + uint_fmt(0, n-1) ;
    for (size_t i = 1 ; i < n ; i++)
      fmtlen += uint_fmt(0, i) + 2 + pmatch[i].rm_eo - pmatch[i].rm_so ;
    {
      size_t m = 0 ;
      char fmt[fmtlen] ;
      fmt[m++] = '#' ; fmt[m++] = '=' ;
      m += uint_fmt(fmt + m, n-1) ;
      fmt[m++] = 0 ;
      fmt[m++] = '0' ; fmt[m++] = '=' ;
      memcpy(fmt + m, expr, exprlen + 1) ; m += exprlen + 1 ;
      for (size_t i = 1 ; i < n ; i++)
      {
        m += uint_fmt(fmt + m, i) ;
        fmt[m++] = '=' ;
        memcpy(fmt + m, s + pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so) ;
        m += pmatch[i].rm_eo - pmatch[i].rm_so ;
        fmt[m++] = 0 ;
      }
      xmexec0_n(argv, fmt, fmtlen, n+1) ;
    }
  }
  else xexec0(argv) ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  static gol_bool const rgolb[] =
  {
    { .so = 'S', .lo = "noshell", .clear = GOLB_SHELL, .set = 0 },
    { .so = 's', .lo = "shell", .clear = 0, .set = GOLB_SHELL },
    { .so = 'E', .lo = "ere", .clear = GOLB_BRE, .set = 0 },
    { .so = 'e', .lo = "bre", .clear = 0, .set = GOLB_BRE },
    { .so = 'n', .lo = "nosub", .clear = GOLB_SUB, .set = 0 },
    { .so = 'N', .lo = "sub", .clear = 0, .set = GOLB_SUB },
    { .so = 'I', .lo = "noicase", .clear = GOLB_ICASE, .set = 0 },
    { .so = 'i', .lo = "icase", .clear = 0, .set = GOLB_ICASE },
  } ;
  uint64_t wgolb = 0 ;
  int argc1 ;
  unsigned int i = 0 ;
  char const *s ;
  PROG = "case" ;

  {
    unsigned int golc = gol_main(argc, argv, rgolb, sizeof(rgolb)/sizeof(gol_bool), 0, 0, &wgolb, 0) ;
    argc -= golc ; argv += golc ;
  }
  if (argc-- < 2) dieusage() ;
  s = *argv++ ;

  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief(100, "unterminated case block") ;

  while (i < argc1)
  {
    char const *expr = argv[i++] ;
    int argc2 ;
    if (i == argc1) strerr_dief(100, "malformed case block") ;
    argc2 = el_semicolon(argv + i) ;
    if (i + argc2 >= argc1) strerr_dief(100, "unterminated regex block") ;
    if (wgolb & GOLB_SHELL)
    {
      int r = fnmatch(expr, s, (wgolb & GOLB_BRE ? FNM_NOESCAPE : 0) | (wgolb & GOLB_ICASE ? FNM_PERIOD : 0) | (wgolb & GOLB_SUB ? FNM_PATHNAME : 0)) ;
      if (!r)
      {
        argv[i + argc2] = 0 ;
        xexec0(argv + i) ;
      }
      else if (r != FNM_NOMATCH)
        strerr_warnw("invalid fnmatch pattern: ", expr) ;
    }
    else
    {
      regex_t re ;
      {
        int r ;
        size_t len = strlen(expr) ;
        char tmp[len+3] ;
        tmp[0] = '^' ;
        memcpy(tmp + 1, expr, len) ;
        tmp[1+len] = '$' ;
        tmp[2+len] = 0 ;
        r = regcomp(&re, tmp, (wgolb & GOLB_BRE ? 0 : REG_EXTENDED) | (wgolb & GOLB_ICASE ? REG_ICASE : 0) | (wgolb & GOLB_SUB ? 0 : REG_NOSUB)) ;
        if (r)
        {
          char buf[256] ;
          regerror(r, &re, buf, 256) ;
          strerr_diefu(r == REG_ESPACE ? 111 : 100, "regcomp \"^", argv[i-1], "$\": ", buf) ;
        }
      }
      {
        regmatch_t pmatch[re.re_nsub && (wgolb & GOLB_SUB) ? re.re_nsub + 1 : 1] ;
        int r = regexec(&re, s, re.re_nsub + 1, pmatch, 0) ;
        if (!r)
        {
          argv[i + argc2] = 0 ;
          case_execit(argv + i, expr, s, pmatch, wgolb & GOLB_SUB ? 1 + re.re_nsub : 0) ;
        }
        if (r != REG_NOMATCH)
        {
          char buf[256] ;
          regerror(r, &re, buf, 256) ;
          strerr_diefu(111, "match string \"", s, "\" against regex \"", expr, "\": ", buf) ;
        }
      }
      regfree(&re) ;
    }
    i += argc2 + 1 ;
  }
  xexec0(argv + argc1 + 1) ;
}
