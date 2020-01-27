/* ISC license. */

#include <sys/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <execline/execline.h>

#define USAGE "runblock [ -P ] [ -n argshift ] [ -r ] n cmd..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv, char const *const *envp)
{
  genalloc v = GENALLOC_ZERO ; /* array of char const * */
  unsigned int n, sharp ;
  unsigned int m = 0 ;
  unsigned int strict = el_getstrict() ;
  int flagnopop = 0, flagr = 0 ;
  PROG = "runblock" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "Prn:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'P' : flagnopop = 1 ; break ;
        case 'r' : flagr = 1 ; break ;
        case 'n' : if (!uint0_scan(l.arg, &m)) dieusage() ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc--) dieusage() ;
  if (!uint0_scan(*argv++, &n) || (!n && !flagr)) dieusage() ;

  {
    char const *x = env_get2(envp, "#") ;
    if (!x) strerr_dienotset(100, "#") ;
    if (!uint0_scan(x, &sharp)) strerr_dieinvalid(100, "#") ;
  }

 /* Skip n-1 blocks (n if flagr) */
  {
    unsigned int i = 1 ;
    for (; i < n + flagr ; i++)
    {
      unsigned int base = m ;
      for (;;)
      {
        char const *x ;
        char fmt[UINT_FMT] ;
        if (++m > sharp) strerr_dief1x(100, "too few arguments") ;
        fmt[uint_fmt(fmt, m)] = 0 ;
        x = env_get2(envp, fmt) ;
        if (!x) strerr_dienotset(100, fmt) ;
        if ((x[0] == EXECLINE_BLOCK_END_CHAR) && (!EXECLINE_BLOCK_END_CHAR || !x[1])) break ;
        if ((x[0] != EXECLINE_BLOCK_QUOTE_CHAR) && strict)
        {
          char fmtb[UINT_FMT] ;
          char fmti[UINT_FMT] ;
          fmti[uint_fmt(fmti, i)] = 0 ;
          fmtb[uint_fmt(fmtb, m - base)] = 0 ;
          if (strict == 1)
            strerr_warnw6x("unquoted positional ", x, " at block ", fmti, " position ", fmtb) ;
          else
            strerr_dief6x(100, "unquoted positional ", x, " at block ", fmti, " position ", fmtb) ;
        }
      }
    }
  }

 /* First put args, if any, into v */

  if (!genalloc_ready(char const *, &v, argc))
    strerr_diefu1sys(111, "genalloc_ready") ;
  for (unsigned int i = 0 ; i < (unsigned int)argc ; i++)
    genalloc_append(char const *, &v, argv + i) ;

  if (flagr)  /* put remainder envvars into v */
  {
    if (++m > sharp) return 0 ;
    if (!genalloc_readyplus(char const *, &v, sharp - m + 2))
      strerr_diefu1sys(111, "genalloc_readyplus") ;
    for (; m <= sharp ; m++)
    {
      char const *x ;
      char fmt[UINT_FMT] ;
      fmt[uint_fmt(fmt, m)] = 0 ;
      x = env_get2(envp, fmt) ;
      if (!x) strerr_dienotset(100, fmt) ;
      genalloc_append(char const *, &v, &x) ;
    }
  }
  else  /* put envvars from nth block into v */
  {
    unsigned int base = m ;
    for (;;)
    {
      char const *x ;
      char fmt[UINT_FMT] ;
      if (++m > sharp) strerr_dief1x(100, "too few arguments") ;
      fmt[uint_fmt(fmt, m)] = 0 ;
      x = env_get2(envp, fmt) ;
      if (!x) strerr_dienotset(100, fmt) ;
      if ((x[0] == EXECLINE_BLOCK_END_CHAR) && (!EXECLINE_BLOCK_END_CHAR || !x[1])) break ;
      if (x[0] != EXECLINE_BLOCK_QUOTE_CHAR)
      {
        if (strict)
        {
          char fmtb[UINT_FMT] ;
          char fmtn[UINT_FMT] ;
          fmtn[uint_fmt(fmtn, n)] = 0 ;
          fmtb[uint_fmt(fmtb, m - base)] = 0 ;
          if (strict == 1)
            strerr_warnw6x("unquoted positional ", x, " at block ", fmtn, " position ", fmtb) ;
          else
            strerr_dief6x(100, "unquoted positional ", x, " at block ", fmtn, " position ", fmtb) ;
        }
      }
      else x++ ;
      if (!genalloc_append(char const *, &v, &x)) strerr_diefu1sys(111, "envalloc_catb") ;
    }
  }

  {
    char const *z = 0 ;
    if (!genalloc_append(char const *, &v, &z))
      strerr_diefu1sys(111, "genalloc_append") ;
  }

  if (flagnopop)  /* exec now */
    xpathexec_run(genalloc_s(char const *, &v)[0], genalloc_s(char const *, &v), envp) ;
  else  /* popenv, then exec */
  {
    char const *list[11] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "#" } ;
    size_t envlen = env_len(envp) ;
    int popped = el_popenv(&satmp, envp, envlen, list, 11) ;
    if (popped < 0) strerr_diefu1sys(111, "pop environment") ;
    else
    {
      char const *w[envlen - popped + 1] ;
      if (!env_make(w, envlen - popped, satmp.s, satmp.len))
        strerr_diefu1sys(111, "env_make") ;
      w[envlen - popped] = 0 ;
      xpathexec_run(genalloc_s(char const *, &v)[0], genalloc_s(char const *, &v), w) ;
    }
  }
}
