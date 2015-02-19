/* ISC license. */

#include <sys/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/ushort.h>
#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forx [ -p | -o okcode,okcode,... | -x breakcode,breakcode,... ] var { values... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

static int isok (unsigned short *tab, unsigned int n, int code)
{
  register unsigned int i = 0 ;
  for (; i < n ; i++) if ((unsigned short)code == tab[i]) break ;
  return i < n ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  char const *x ;
  int argc1 ;
  unsigned short okcodes[256] ;
  unsigned int nbc = 0 ;
  int flagpar = 0, not = 1 ;
  PROG = "forx" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "epo:x:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'e' : break ; /* compat */
        case 'p' : flagpar = 1 ; break ;
        case 'o' :
          not = 0 ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        case 'x' :
          not = 1 ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  if (argc < 2) dieusage() ;
  x = argv[0] ; if (!*x) dieusage() ;
  argv++ ; argc-- ;
  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (!argc1 || (argc1 + 1 == argc)) return 0 ;
  {
    unsigned int envlen = env_len(envp) ;
    unsigned int varlen = str_len(x) ;
    unsigned int i = 0 ;
    pid_t pids[flagpar ? argc1 : 1] ;
    char const *newenv[envlen + 2] ;

    for (; i < (unsigned int)argc1 ; i++)
    {
      pid_t pid ;
      unsigned int vallen = str_len(argv[i]) ;
      char modif[varlen + vallen + 2] ;
      byte_copy(modif, varlen, x) ;
      modif[varlen] = '=' ;
      byte_copy(modif + varlen + 1, vallen, argv[i]) ;
      modif[varlen + vallen + 1] = 0 ;
      if (!env_merge(newenv, envlen + 2, envp, envlen, modif, varlen + vallen + 2))
        strerr_diefu1sys(111, "build new environment") ;
      pid = el_spawn0(argv[argc1+1], argv + argc1 + 1, newenv) ;
      if (!pid) strerr_diefu2sys(111, "spawn ", argv[argc1+1]) ;
      if (flagpar) pids[i] = pid ;
      else
      {
        int wstat ;
        if (wait_pid(pid, &wstat) == -1)
          strerr_diefu2sys(111, "wait for ", argv[argc1+1]) ;
        if (not == isok(okcodes, nbc, wait_estatus(wstat)))
          return wait_estatus(wstat) ;
      }
    }
    if (flagpar)
      if (!waitn(pids, argc1)) strerr_diefu1sys(111, "waitn") ;
  }
  return 0 ;
}
