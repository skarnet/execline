/* ISC license. */

#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <skalibs/uint64.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#define USAGE "withstdinas [ -i | -D default ] [ -n ] var remainder..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv, char const *const *envp)
{
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  stralloc modif = STRALLOC_ZERO ;
  unsigned int modifstart ;
  int insist = 0, chomp = 0, reapit = 0 ;
  char const *def = 0 ;
  PROG = "withstdinas" ;
  for (;;)
  {
    register int opt = subgetopt_r(argc, argv, "einD:!", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'i' : insist = 1 ; break ;
      case 'n' : chomp = 1 ; break ;
      case 'D' : def = localopt.arg ; break ;
      case '!' : reapit = 1 ; break ;
      default : dieusage() ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;

  if (!argc) dieusage() ;
  if (!*argv[0] || strchr(argv[0], '=')) strerr_dief1x(100, "invalid variable name") ;
  if (!stralloc_catb(&modif, "!", 2)
   || !stralloc_cats(&modif, argv[0])
   || !stralloc_catb(&modif, "=", 1))
    strerr_diefu1sys(111, "stralloc_catb") ;
  modifstart = modif.len ;
  if (!slurp(&modif, 0)) strerr_diefu1sys(111, "slurp") ;
  if (reapit)
  {
    char const *x = env_get2(envp, "!") ;
    if (x)
    {
      uint64 pid ;
      int wstat ;
      if (!uint640_scan(x, &pid)) strerr_dieinvalid(100, "!") ;
      if (waitpid(pid, &wstat, 0) < 0)
        strerr_diefu1sys(111, "waitpid") ;
      if (wait_estatus(wstat))
      {
        if (insist)
          if (WIFSIGNALED(wstat)) strerr_dief1x(wait_estatus(wstat), "child process crashed") ;
          else strerr_dief1x(wait_estatus(wstat), "child process exited non-zero") ;
        else if (def)
        {
          modif.len = modifstart ;
          if (!stralloc_cats(&modif, def)) strerr_diefu1sys(111, "stralloc_catb") ;
        }
      }
    }
  }
  if (!stralloc_0(&modif)) strerr_diefu1sys(111, "stralloc_catb") ;
  {
    unsigned int reallen = str_len(modif.s + 2) ;
    if (reallen < modif.len - 3)
    {
      if (insist)
        strerr_dief1x(1, "stdin contained a null character") ;
      else if (def)
      {
        modif.len = modifstart ;
        if (!stralloc_catb(&modif, def, str_len(def)+1))
          strerr_diefu1sys(111, "stralloc_catb") ;
        strerr_warnw2x("stdin contained a null character", " - using default instead") ;
      }
      else
        modif.len = reallen + 3 ;
    }
    if (chomp && (modif.s[modif.len - 2] == '\n'))
      modif.s[--modif.len - 1] = 0 ;
  }
  if (!argv[1]) return 0 ;
  pathexec_r(argv + 1, envp, env_len(envp), modif.s + !reapit * 2, modif.len - !reapit * 2) ;
  strerr_dieexec(111, argv[1]) ;
}
