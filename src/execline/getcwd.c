/* ISC license. */

#include <string.h>

#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#include <execline/execline.h>

#define USAGE "getcwd [ -E | -e ] variable prog..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  int doimport = 0 ;
  stralloc sa = STRALLOC_ZERO ;
  PROG = "getcwd" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "Ee", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'E' : doimport = 1 ; break ;
        case 'e' : doimport = 0 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) dieusage() ;
  if (!argv[0][0] || strchr(argv[0], '=')) strerr_dief1x(100, "invalid variable name") ;

  if (sagetcwd(&sa) < 0 || !stralloc_0(&sa)) strerr_diefu1sys(111, "getcwd") ;
  el_modif_and_exec(argv + 1, argv[0], sa.s, doimport) ;
}
