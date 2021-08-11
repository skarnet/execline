/* ISC license. */

#include <string.h>
#include <regex.h>

#include <skalibs/gccattributes.h>
#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "case [ -e | -E ] [ -n | -N ] [ -i ] value { re1 { prog1... } re2 { prog2... } ... } progdefault... "
#define dieusage() strerr_dieusage(100, USAGE)

static void execit (char const *const *argv, char const *expr, char const *s, regmatch_t const *pmatch, size_t nsub, int flagnosub) gccattr_noreturn ;
static void execit (char const *const *argv, char const *expr, char const *s, regmatch_t const *pmatch, size_t nsub, int flagnosub)
{
  if (flagnosub) xexec0(argv) ;
  else
  {
    size_t exprlen = strlen(expr) ;
    size_t fmtlen = exprlen + 6 ;
    for (size_t i = 0 ; i < nsub ; i++)
      fmtlen += uint_fmt(0, i+1) + 2 + pmatch[i].rm_eo - pmatch[i].rm_so ;
    {
      size_t m = 0 ;
      char fmt[fmtlen] ;
      fmt[m++] = '#' ; fmt[m++] = '=' ;
      m += uint_fmt(fmt + m, nsub) ;
      fmt[m++] = 0 ;
      fmt[m++] = '0' ; fmt[m++] = '=' ;
      memcpy(fmt + m, expr, exprlen + 1) ; m += exprlen + 1 ;
      for (size_t i = 0 ; i < nsub ; i++)
      {
        m += uint_fmt(fmt + m, i+1) ;
        fmt[m++] = '=' ;
        memcpy(fmt + m, s + pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so) ;
        fmt[m++] = 0 ;
      }
      xmexec0_n(argv, fmt, fmtlen, 2 + nsub) ;
    }
  }
}

int main (int argc, char const **argv, char const *const *envp)
{
  int flagextended = 1 ;
  int flagicase = 0 ;
  int flagnosub = 1 ;
  int argc1 ;
  unsigned int i = 0 ;
  char const *s ;
  PROG = "case" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "eEnNi", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'e' : flagextended = 0 ; break ;
        case 'E' : flagextended = 1 ; break ;
        case 'N' : flagnosub = 0 ; break ;
        case 'n' : flagnosub = 1 ; break ;
        case 'i' : flagicase = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc-- < 2) dieusage() ;
  s = *argv++ ;

  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated case block") ;

  while (i < argc1)
  {
    char const *expr = argv[i++] ;
    int argc2 ;
    regex_t re ;
    if (i == argc1) strerr_dief1x(100, "malformed case block") ;
    argc2 = el_semicolon(argv + i) ;
    if (i + argc2 >= argc1) strerr_dief1x(100, "unterminated regex block") ;
    {
      int r ;
      size_t len = strlen(expr) ;
      char tmp[len+3] ;
      tmp[0] = '^' ;
      memcpy(tmp + 1, expr, len) ;
      tmp[1+len] = '$' ;
      tmp[2+len] = 0 ;
      r = regcomp(&re, tmp, (flagextended ? REG_EXTENDED : 0) | (flagicase ? REG_ICASE : 0) | (flagnosub ? REG_NOSUB : 0) | REG_NEWLINE) ;
      if (r)
      {
        char buf[256] ;
        regerror(r, &re, buf, 256) ;
        strerr_diefu4x(r == REG_ESPACE ? 111 : 100, "regcomp \"^", argv[i], "$\": ", buf) ;
      }
    }
    {
      regmatch_t pmatch[re.re_nsub && !flagnosub ? re.re_nsub : 1] ;
      int r = regexec(&re, s, re.re_nsub, pmatch, 0) ;
      if (!r)
      {
        argv[i + argc2] = 0 ;
        execit(argv + i, expr, s, pmatch, flagnosub ? 0 : re.re_nsub, flagnosub) ;
      }
      if (r != REG_NOMATCH)
      {
        char buf[256] ;
        regerror(r, &re, buf, 256) ;
        strerr_diefu6x(111, "match string \"", s, "\" against regex \"", expr, "\": ", buf) ;
      }
    }
    regfree(&re) ;
    i += argc2 + 1 ;
  }
  xexec0(argv + argc1 + 1) ;
}
