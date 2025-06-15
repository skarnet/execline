/* ISC license. */

#include <string.h>
#include <errno.h>
#include <signal.h>

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/sig.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/netstring.h>

#include <execline/execline.h>

#define USAGE "forstdin [ -E | -e ] [ -p | -P maxpar | -o okcode,okcode,... | -x breakcode,breakcode,... ] [ -N | -n ] [ -C | -c ] [ -0 | -d delim ] var command..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv)
{
  el_forx_pidinfo_t pidinfo = EL_FORX_PIDINFO_ZERO ;
  stralloc value = STRALLOC_ZERO ;
  sigset_t emptyset ;
  char const *delim = "\n" ;
  size_t delimlen = 1 ;
  size_t nbc = 0 ;
  unsigned int maxpar = 1 ;
  unsigned short okcodes[256] ;
  int crunch = 0, chomp = 1, not = 1, eofcode = 1, doimport = 0 ;
  PROG = "forstdin" ;
  el_forx_pidinfo = &pidinfo ;
  sigemptyset(&emptyset) ;

  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "pP:NnCc0d:o:x:Ee", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' : maxpar = 10000 ; break ;
        case 'P' : if (!uint0_scan(l.arg, &maxpar)) dieusage() ; break ;
        case 'N' : chomp = 0 ; break ;
        case 'n' : chomp = 1 ; break ;
        case 'C' : crunch = 1 ; break ;
        case 'c' : crunch = 0 ; break ;
        case '0' : delim = "" ; delimlen = 1 ; break ;
        case 'd' : delim = l.arg ; delimlen = strlen(delim) ; break ;
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
  if (maxpar < 1) maxpar = 1 ;
  if (maxpar >= 10000) maxpar = 10000 ;
  if (!sig_catch(SIGCHLD, &el_forx_sigchld_handler))
    strerr_diefu1sys(111, "install SIGCHLD handler") ;

  pid_t pidtab[maxpar] ;
  pidinfo.tab = pidtab ;
  sig_block(SIGCHLD) ;
  for (;;)
  {
    value.len = 0 ;
    if (delimlen)
    {
      int r = skagetlnsep(buffer_0, &value, delim, delimlen) ;
      if (!r) break ;
      else if (r < 0)
      {
        if (errno != EPIPE) strerr_diefu1sys(111, "skagetlnsep") ;
        if (chomp) break ;
      }
      if (crunch && value.len == 1) continue ;
      if (chomp) value.len-- ;
    }
    else
    {
      size_t unread = 0 ;
      if (netstring_get(buffer_0, &value, &unread) <= 0)
      {
        if (netstring_okeof(buffer_0, unread)) break ;
        else strerr_diefu1sys(111, "netstring_get") ;
      }
    }
    eofcode = 0 ;
    if (!stralloc_0(&value)) strerr_diefu1sys(111, "stralloc_0") ;
    pidtab[pidinfo.len] = el_modif_and_spawn(argv + 1, argv[0], value.s, doimport) ;
    if (!pidtab[pidinfo.len]) strerr_diefu2sys(111, "spawn ", argv[1]) ;
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
  return eofcode ;
}
