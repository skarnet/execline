/* ISC license. */

#include <skalibs/cspawn.h>

#include <execline/config.h>
#include <execline/execline.h>

pid_t el_spawn0 (char const *prog, char const *const *argv, char const *const *envp)
{
  if (!argv[0])
  {
    static char const *const newargv[3] = { EXECLINE_BINPREFIX "exit", "0", 0 } ;
    return cspawn(newargv[0], newargv, envp, 0, 0, 0) ;
  }
  else return cspawn(prog, argv, envp, 0, 0, 00) ;
}
