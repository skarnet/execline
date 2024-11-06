/* ISC license. */

#include <skalibs/cspawn.h>

#include <execline/config.h>
#include <execline/execline.h>

pid_t el_spawn0 (char const *prog, char const *const *argv, char const *const *envp)
{
  if (!argv[0]) argv = el_trueargv ;
  return cspawn(prog, argv, envp, 0, 0, 0) ;
}
