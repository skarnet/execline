/* ISC license. */

#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>

#define USAGE "withstdinas [ -i | -I | -D default ] [ -n ] var remainder..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv, char const *const *envp)
{
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  stralloc modif = STRALLOC_ZERO ;
  size_t modifstart ;
  int insist = 0, chomp = 0 ;
  char const *def = 0 ;
  PROG = "withstdinas" ;
  for (;;)
  {
    int opt = subgetopt_r(argc, argv, "iInD:", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'i' : insist = 2 ; break ;
      case 'I' : insist = 1 ; break ;
      case 'n' : chomp = 1 ; break ;
      case 'D' : def = localopt.arg ; break ;
      default : dieusage() ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;

  if (!argc) dieusage() ;
  if (!*argv[0] || strchr(argv[0], '=')) strerr_dief1x(100, "invalid variable name") ;
  if (!stralloc_cats(&modif, argv[0]) || !stralloc_catb(&modif, "=", 1))
    strerr_diefu1sys(111, "stralloc_catb") ;
  modifstart = modif.len ;
  if (!slurp(&modif, 0)) strerr_diefu1sys(111, "slurp") ;
  if (!stralloc_0(&modif)) strerr_diefu1sys(111, "stralloc_catb") ;
  {
    size_t reallen = strlen(modif.s) ;
    if (reallen < modif.len - 1)
    {
      if (insist >= 2)
        strerr_dief1x(1, "stdin contained a null character") ;
      else if (insist)
      {
        modif.len = modifstart ;
        modif.s[modif.len - 1] = 0 ;
        chomp = 0 ;
      }
      else if (def)
      {
        modif.len = modifstart ;
        if (!stralloc_catb(&modif, def, strlen(def)+1))
          strerr_diefu1sys(111, "stralloc_catb") ;
        strerr_warnw2x("stdin contained a null character", " - using default instead") ;
      }
      else modif.len = reallen + 1 ;
    }
    if (chomp && (modif.s[modif.len - 2] == '\n'))
      modif.s[--modif.len - 1] = 0 ;
  }
  if (!argv[1]) return 0 ;
  xpathexec_r(argv + 1, envp, env_len(envp), modif.s, modif.len) ;
}
