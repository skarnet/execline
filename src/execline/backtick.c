/* ISC license. */

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

#define USAGE "` [ -i | -I | -D default ] [ -n ] var { prog... } remainder..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv, char const *const *envp)
{
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  pid_t pid ;
  int argc1, fdwstat ;
  stralloc modif = STRALLOC_ZERO ;
  size_t modifstart ;
  int insist = 0, chomp = 0 ;
  char const *def = 0 ;
  PROG = "`" ;
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

  if (argc < 2) dieusage() ;
  if (!*argv[0]) strerr_dief1x(100, "empty variable not accepted") ;
  if (!stralloc_cats(&modif, argv[0]) || !stralloc_catb(&modif, "=", 1))
    strerr_diefu1sys(111, "stralloc_catb") ;
  modifstart = modif.len ;
  argc-- ; argv++ ;
  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;

  argv[argc1] = 0 ;
  pid = child_spawn1_pipe(argv[0], argv, envp, &fdwstat, 1) ;
  if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
  if (!slurp(&modif, fdwstat)) strerr_diefu1sys(111, "slurp") ;
  close(fdwstat) ;
  if (wait_pid(pid, &fdwstat) < 0) strerr_diefu1sys(111, "wait_pid") ;

  if (wait_status(fdwstat))
  {
    if (insist >= 2)
      if (WIFSIGNALED(fdwstat)) strerr_dief1x(111, "child process crashed") ;
      else strerr_dief1x(WEXITSTATUS(fdwstat), "child process exited non-zero") ;
    else if (insist)
    {
      modif.len = modifstart - 1 ;
      chomp = 0 ;
    }
    else if (def)
    {
      modif.len = modifstart ;
      if (!stralloc_cats(&modif, def)) strerr_diefu1sys(111, "stralloc_catb") ;
    }
  }
  if (argc1 == argc - 1) return 0 ;
  if (!stralloc_0(&modif)) strerr_diefu1sys(111, "stralloc_catb") ;
  {
    size_t reallen = strlen(modif.s) ;
    if (reallen < modif.len - 1)
    {
      if (insist >= 2)
        strerr_dief1x(1, "child process output contained a null character") ;
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
        strerr_warnw2x("child process output contained a null character", " - using default instead") ;
      }
      else modif.len = reallen + 1 ;
    }
    if (chomp && (modif.s[modif.len - 2] == '\n'))
      modif.s[--modif.len - 1] = 0 ;
  }
  xpathexec_r(argv + argc1 + 1, envp, env_len(envp), modif.s, modif.len) ;
}
