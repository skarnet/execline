/* ISC license. */

#include <string.h>

#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/types.h>

#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forx [ -E | -e ] [ -p ] [ -o okcode,okcode,... | -x breakcode,breakcode,... ] var { values... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

static int isok (unsigned short const *tab, unsigned int n, int code)
{
  unsigned int i = 0 ;
  for (; i < n ; i++) if ((unsigned short)code == tab[i]) break ;
  return i < n ;
}

static int waitn_code (unsigned short const *tab, unsigned int nbc, pid_t *pids, unsigned int n, int not)
{
  int ok = 1 ;
  while (n)
  {
    int wstat ;
    unsigned int i = 0 ;
    pid_t pid = wait_nointr(&wstat) ;
    if (pid < 0) return -1 ;
    for (; i < n ; i++) if (pid == pids[i]) break ;
    if (i < n)
    {
      if (not == isok(tab, nbc, wait_estatus(wstat))) ok = 0 ;
      pids[i] = pids[--n] ;
    }
  }
  return ok ;
}

int main (int argc, char const **argv)
{
  char const *var ;
  unsigned short okcodes[256] ;
  size_t nbc = 0 ;
  int flagpar = 0, not = 1, doimport = 0 ;
  unsigned int argc1 ;
  PROG = "forx" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "po:x:Ee", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' : flagpar = 1 ; break ;
        case 'o' :
          not = 0 ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        case 'x' :
          not = 1 ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        case 'E' : doimport = 1 ; break ;
        case 'e' : doimport = 0 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  if (argc < 2) dieusage() ;
  if (!argv[0][0] || strchr(argv[0], '=')) strerr_dief1x(100, "invalid variable name") ;
  var = *argv++ ; argc-- ;
  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (!argc1 || (argc1 + 1 == argc)) return 0 ;

  {
    pid_t pids[flagpar ? argc1 : 1] ;
    for (unsigned int i = 0 ; i < argc1 ; i++)
    {
      pid_t pid = el_modif_and_spawn(argv + argc1 + 1, var, argv[i], doimport) ;
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
    {
      int r = waitn_code(okcodes, nbc, pids, argc1, not) ;
      if (r < 0) strerr_diefu1sys(111, "waitn") ;
      else if (!r) return 1 ;
    }
  }
  return 0 ;
}
