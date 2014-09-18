/* ISC license. */

#include <sys/types.h>
#include <sys/wait.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/ushort.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

#define USAGE "loopwhilex [ -n ] [ -x exitcode,exitcode,... ] prog..."
#define dieusage() strerr_dieusage(100, USAGE)

static int isbreak (unsigned short *tab, unsigned int n, int code)
{
  register unsigned int i = 0 ;
  for (; i < n ; i++) if ((unsigned short)code == tab[i]) break ;
  return i < n ;
}

int main (int argc, char const *const *argv, char const *const *envp)
{
  int wstat ;
  int not = 0, cont = 1 ;
  unsigned short breakcodes[256] ;
  unsigned int nbc = 0 ;
  PROG = "loopwhilex" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "nx:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : not = 1 ; break ;
        case 'x' :
          if (!ushort_scanlist(breakcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) dieusage() ;

  if (!nbc)
  {
    breakcodes[0] = 0 ;
    nbc = 1 ;
    not = !not ;
  }

  while (cont)
  {
    pid_t pid = el_spawn0(argv[0], argv, envp) ;
    if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
    if (wait_pid(pid, &wstat) < 0) strerr_diefu1sys(111, "wait_pid") ;
    cont = not == isbreak(breakcodes, nbc, wait_status(wstat)) ;
  }
  return WIFSIGNALED(wstat) ? WTERMSIG(wstat) : 0 ;
}
