/* ISC license. */

#include <errno.h>
#include <skalibs/uint.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <execline/execline.h>
#include "exlsn.h"

#define USAGE "execlineb [ -p | -P | -S nmin ] [ -q | -w | -W ] [ -c commandline ] script args"

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
  stralloc sa = STRALLOC_ZERO ;
  stralloc modif = STRALLOC_ZERO ;
  int nc ;
  int flagstrict = -1 ;
  unsigned int nmin = 0 ;
  char const *stringarg = 0 ;
  char const *dollar0 = *argv ;
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
        case 'c' : stringarg = l.arg ; break ;
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
  if (stringarg) nc = el_parse_from_string(&sa, stringarg) ;
  else
  {
    char buf[BUFFER_INSIZE] ;
    buffer b ;
    int fd ;
    int e ;
    if (!argc--) strerr_dieusage(100, USAGE) ;
    dollar0 = *argv++ ;
    fd = open_readb(dollar0) ;
    if (fd < 0) strerr_diefu3sys(111, "open ", dollar0, " for reading") ;
    buffer_init(&b, &fd_readsv, fd, buf, BUFFER_INSIZE) ;
    nc = el_parse_from_buffer(&sa, &b) ;
    e = errno ;
    fd_close(fd) ;
    errno = e ;
  }

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
