/* ISC license. */

#include <skalibs/djbunix.h>

#include <execline/config.h>
#include <execline/execline.h>

pid_t el_spawn0 (char const *prog, char const *const *argv, char const *const *envp)
{
  if (!argv[0])
  {
    static char const *const newargv[3] = { EXECLINE_BINPREFIX "exit", "0", 0 } ;
    return child_spawn0(newargv[0], newargv, 0) ;
  }
  else return child_spawn0(prog, argv, envp) ;
}
