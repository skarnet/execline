/* ISC license. */

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "backtick [ -i | -I | -x | -D default ] [ -N | -n ] [ -E | -e ] var { prog... } remainder..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv, char const *const *envp)
{
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  int argc1, fdwstat ;
  stralloc value = STRALLOC_ZERO ;
  char const *var ;
  char const *val ;
  int insist = 2, chomp = 1, doimport = 0 ;
  char const *def = 0 ;
  PROG = "backtick" ;
  for (;;)
  {
    int opt = subgetopt_r(argc, argv, "iINnxD:Ee", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'i' : insist = 2 ; break ;
      case 'I' : insist = 0 ; break ;
      case 'N' : chomp = 0 ; break ;
      case 'n' : chomp = 1 ; break ;
      case 'x' : insist = 1 ; def = 0 ; break ;
      case 'D' : insist = 1 ; def = localopt.arg ; break ;
      case 'E' : doimport = 1 ; break ;
      case 'e' : doimport = 0 ; break ;
      default : dieusage() ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;

  if (argc < 2) dieusage() ;
  if (!argv[0][0] || strchr(argv[0], '=')) strerr_dief1x(100, "invalid variable name") ;
  argc-- ; var = *argv++ ;
  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  argv[argc1] = 0 ;

  {
    pid_t pid = child_spawn1_pipe(argv[0], argv, envp, &fdwstat, 1) ;
    if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
    if (!slurp(&value, fdwstat) || !stralloc_0(&value))
      strerr_diefu1sys(111, "slurp") ;
    close(fdwstat) ;
    if (wait_pid(pid, &fdwstat) < 0) strerr_diefu1sys(111, "wait_pid") ;
  }

  val = value.s ;
  if (wait_status(fdwstat))
  {
    if (insist >= 2)
      strerr_dief1x(wait_estatus(fdwstat), WIFSIGNALED(fdwstat) ? "child process crashed" : "child process exited non-zero") ;
    else if (insist) val = def ;
  }
  else if (strlen(value.s) < value.len - 1)
  {
    if (insist >= 2)
      strerr_dief1x(124, "child process output contained a null character") ;
    else if (insist)
    {
      val = def ;
      strerr_warnw1x("child process output contained a null character") ;
    }
    else value.len = strlen(value.s) + 1 ;
  }
  else insist = 0 ;
  if (!insist && chomp && (value.s[value.len - 2] == '\n'))
    value.s[--value.len - 1] = 0 ;
  el_modif_and_exec(argv + argc1 + 1, var, val, doimport) ;
}
