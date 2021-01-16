/* ISC license. */

#include <string.h>

#include <skalibs/gccattributes.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/env.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "emptyenv [ -p | -c | -o | -P ] prog..."

static void cleanupenv (char const *const *, char const *const *) gccattr_noreturn ;
static void cleanupenv (char const *const *argv, char const *const *envp)
{
  stralloc sa = STRALLOC_ZERO ;
  if (!env_mexec("!", 0) || !env_mexec("?", 0)) goto err ;
  for (; *envp ; envp++)
  {
    char const *s = *envp ;
    sa.len = 0 ;
    if (!strncmp(s, "ELGETOPT_", 9)
     || !strncmp(s, "EXECLINE_", 9)
     || !strncmp(s, "FD", 2)
     || (s[0] == '#')
     || ((s[0] >= '0') && (s[0] <= '9')))
      if (!stralloc_catb(&sa, s, str_chr(s, '='))
       || !stralloc_0(&sa)
       || !env_mexec(sa.s, 0))
        goto err ;
  }
  stralloc_free(&sa) ;
  xmexec(argv) ;
err:
  strerr_diefu1sys(111, "clean up environment") ;
}

int main (int argc, char const *const *argv, char const *const *envp)
{
  int flagpath = 0, flagcleanup = 0, flagopt = 0, flagpos = 0 ;
  PROG = "emptyenv" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "pcoP", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' : flagpath = 1 ; break ;
        case 'c' : flagcleanup = 1 ; break ;
        case 'o' : flagopt = 1 ; break ;
        case 'P' : flagpos = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  if (flagcleanup) cleanupenv(argv, envp) ;
  else if (!flagopt && !flagpos)
  {
    char const *newenv[2] = { 0, 0 } ;
    if (flagpath)
      for (; *envp ; envp++)
        if (!strncmp(*envp, "PATH=", 5))
        {
          newenv[0] = *envp ;
          break ;
        }
    xexec_e(argv, newenv) ;
  }
  else
  {
    static char const *const list[12] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "#", "ELGETOPT_" } ;
    stralloc sa = STRALLOC_ZERO ;
    size_t envlen = env_len(envp) ;
    int n = el_popenv(&sa, envp, envlen, flagpos ? list : list + 11, 11 * flagpos + flagopt) ;
    if (n < 0) strerr_diefu1sys(111, "pop current execline environment") ;
    {
      char const *v[envlen - n + 1] ;
      if (!env_make(v, envlen-n, sa.s, sa.len)) strerr_diefu1sys(111, "env_make") ;
      v[envlen-n] = 0 ;
      xexec_e(argv, v) ;
    }
  }
}
