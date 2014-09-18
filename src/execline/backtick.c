/* ISC license. */

#include <sys/types.h>
#include <unistd.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

#define USAGE "backtick [ -i ] [ -n ] var { prog... } remainder..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv, char const *const *envp)
{
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  int argc1 ;
  stralloc modif = STRALLOC_ZERO ;
  int insist = 0, chomp = 0 ;
  PROG = "backtick" ;
  for (;;)
  {
    register int opt = subgetopt_r(argc, argv, "ein", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'i' : insist = 1 ; break ;
      case 'n' : chomp = 1 ; break ;
      case 'e' : break ; /* compat */
      default : dieusage() ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;

  if (argc < 2) dieusage() ;
  if (!*argv[0]) strerr_dief1x(100, "empty variable not accepted") ;
  if (!stralloc_cats(&modif, argv[0]) || !stralloc_catb(&modif, "=", 1))
    strerr_diefu1sys(111, "stralloc_catb") ;
  argc-- ; argv++ ;
  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;

  {
    int p[2] ;
    pid_t pid ;
    if (pipe(p) < 0) strerr_diefu1sys(111, "pipe") ;
    pid = fork() ;
    switch (pid)
    {
      case -1: strerr_diefu1sys(111, "fork") ;
      case 0:
        argv[argc1] = 0 ;
        fd_close(p[0]) ;
        PROG = "backtick (child)" ;
        if (fd_move(1, p[1]) < 0) strerr_diefu1sys(111, "fd_move") ;
        pathexec_run(argv[0], argv, envp) ;
        strerr_dieexec(111, argv[0]) ;
    }
    fd_close(p[1]) ;
    if (!slurp(&modif, p[0])) strerr_diefu1sys(111, "slurp") ;
    fd_close(p[0]) ;
    if (wait_pid(pid, &p[0]) < 0) strerr_diefu1sys(111, "wait_pid") ;
    if (insist && wait_status(p[0]))
      strerr_dief1x(wait_status(p[0]), "child process exited non-zero") ;
  }
  if (argc == argc1 - 1) return 0 ;
  if (!stralloc_0(&modif)) strerr_diefu1sys(111, "stralloc_catb") ;
  {
    unsigned int reallen = str_len(modif.s) ;
    if (reallen < modif.len - 1)
    {
      if (insist)
        strerr_dief1x(1, "child process output contained a null character") ;
      else
        modif.len = reallen + 1 ;
    }
    if (chomp && (modif.s[modif.len - 2] == '\n'))
      modif.s[--modif.len - 1] = 0 ;
  }
  pathexec_r(argv + argc1 + 1, envp, env_len(envp), modif.s, modif.len) ;
  strerr_dieexec(111, argv[argc1 + 1]) ;
}
