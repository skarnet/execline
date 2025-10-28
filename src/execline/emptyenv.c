/* ISC license. */

#include <string.h>

#include <skalibs/uint64.h>
#include <skalibs/types.h>
#include <skalibs/posixplz.h>
#include <skalibs/bytestr.h>
#include <skalibs/prog.h>
#include <skalibs/gol.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/env.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "emptyenv [ -p | -l | -c | -o | -P ] prog..."

enum emptyenv_golb_e
{
  GOLB_PATH = 0x01,
  GOLB_LIB = 0x02,
  GOLB_CLEANUP = 0x04,
  GOLB_OPT = 0x08,
  GOLB_POS = 0x10
} ;

int main (int argc, char const *const *argv)
{
  static gol_bool const rgolb[5] =
  {
    { .so = 'p', .lo = "keep-path", .clear = 0, .set = GOLB_PATH },
    { .so = 'l', .lo = "keep-ld-library-path", .clear = 0, .set = GOLB_LIB },
    { .so = 'c', .lo = "cleanup", .clear = 0, .set = GOLB_CLEANUP },
    { .so = 'o', .lo = "pop-elgetopt", .clear = 0, .set = GOLB_OPT },
    { .so = 'P', .lo = "pop-execline", .clear = 0, .set = GOLB_POS }
  } ;
  uint64_t wgolb = 0 ;
  unsigned int golc ;
  PROG = "emptyenv" ;
  golc = gol_main(argc, argv, rgolb, 5, 0, 0, &wgolb, 0) ;
  argc -= golc ; argv += golc ;
  if (!argc) strerr_dieusage(100, USAGE) ;

  if (wgolb & GOLB_CLEANUP)
  {
    static char const *const onebyte = "!?#0" ;
    char *const *envp = environ ;
    for (size_t i = 0 ; onebyte[i] ; i++)
    {
      char s[2] ;
      s[0] = onebyte[i] ;
      s[1] = 0 ;
      if (!env_mexec(s, 0)) goto err ;
    }
    if (!env_mexec("EXECLINE_STRICT", 0)) goto err ;
    for (; *envp ; envp++)
    {
      size_t l ;
      char const *s = *envp ;
      if (!strncmp(s, "ELGETOPT_", 9)) goto add ;
      if (!strncmp(s, "FD", 2))
      {
        unsigned int n ;
        l = uint_scan(s + 2, &n) ;
        if (l && s[2 + l] == '=') goto add ;
      }
      if ((s[0] >= '1') && (s[0] <= '9'))
      {
        unsigned int n ;
        l = uint_scan(s, &n) ;
        if (l && s[l] == '=') goto add ;
      }
      continue ;
     add:
      l = str_chr(s, '=') ;
      if (l)
      {
        char tmp[l+1] ;
        memcpy(tmp, s, l) ;
        tmp[l] = 0 ;
        if (!env_mexec(tmp, 0)) goto err ;
      }
    }
    xmexec(argv) ;
  err:
    strerr_diefu1sys(111, "clean up environment") ;
  }

  else if (wgolb & (GOLB_OPT | GOLB_POS))
  {
    static char const *const list[12] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "#", "ELGETOPT_" } ;
    stralloc sa = STRALLOC_ZERO ;
    size_t envlen = env_len((char const *const *)environ) ;
    int n = el_popenv(&sa, (char const *const *)environ, envlen, wgolb & GOLB_POS ? list : list + 11, (wgolb & GOLB_POS ? 11 : 0) + !!(wgolb & GOLB_OPT)) ;
    if (n < 0) strerr_diefu1sys(111, "pop current execline environment") ;
    {
      char const *v[envlen - n + 1] ;
      if (!env_make(v, envlen-n, sa.s, sa.len)) strerr_diefu1sys(111, "env_make") ;
      v[envlen-n] = 0 ;
      xexec_e(argv, v) ;
    }
  }

  else
  {
    char const *newenv[3] = { 0, 0, 0 } ;
    char *const *envp = environ ;
    unsigned int m = 0 ;
    wgolb &= GOLB_PATH | GOLB_LIB ;
    for (; wgolb && *envp ; envp++)
    {
      if (wgolb & GOLB_PATH && !strncmp(*envp, "PATH=", 5))
      {
         newenv[m++] = *envp ;
         wgolb &= ~GOLB_PATH ;
      }
      if (wgolb & GOLB_LIB && !strncmp(*envp, "LD_LIBRARY_PATH=", 16))
      {
         newenv[m++] = *envp ;
         wgolb &= ~GOLB_LIB ;
      }
    }
    xexec_e(argv, newenv) ;
  }
}
