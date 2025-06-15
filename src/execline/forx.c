/* ISC license. */

#include <string.h>
#include <signal.h>
#include <limits.h>

#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/strerr.h>
#include <skalibs/env.h>
#include <skalibs/sig.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/types.h>

#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forx [ -E | -e ] [ -p | -P maxpar ] [ -o okcode,okcode,... | -x breakcode,breakcode,... ] var { values... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv)
{
  char const *var ;
  el_forx_pidinfo_t pidinfo = EL_FORX_PIDINFO_ZERO ;
  sigset_t emptyset ;
  unsigned short okcodes[256] ;
  size_t nbc = 0 ;
  int not = 1, doimport = 0 ;
  unsigned int maxpar = 1, argc1 ;
  PROG = "forx" ;
  el_forx_pidinfo = &pidinfo ;
  sigemptyset(&emptyset) ;

  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "pP:o:x:Ee", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' : maxpar = UINT_MAX ; break ;
        case 'P' : if (!uint0_scan(l.arg, &maxpar)) dieusage() ; break ;
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
  if (maxpar < 1) maxpar = 1 ;
  if (maxpar > argc1) maxpar = argc1 ;
  if (!sig_catch(SIGCHLD, &el_forx_sigchld_handler))
    strerr_diefu1sys(111, "install SIGCHLD handler") ;

  {
    pid_t pidtab[maxpar] ;
    pidinfo.tab = pidtab ;
    sig_block(SIGCHLD) ;

    for (unsigned int i = 0 ; i < argc1 ; i++)
    {
      pidtab[pidinfo.len] = el_modif_and_spawn(argv + argc1 + 1, var, argv[i], doimport) ;
      if (!pidtab[pidinfo.len]) strerr_diefu2sys(111, "spawn ", argv[argc1+1]) ;
      pidinfo.len++ ;
      while (pidinfo.len >= maxpar)
      {
        unsigned int oldlen = pidinfo.len ;
        sigsuspend(&emptyset) ;
        if (pidinfo.len < oldlen && maxpar == 1 && not == el_forx_isok(okcodes, nbc, wait_estatus(pidinfo.wstat)))
          return wait_estatus(pidinfo.wstat) ;
      }
    }
    while (pidinfo.len) sigsuspend(&emptyset) ;
  }
  return 0 ;
}
