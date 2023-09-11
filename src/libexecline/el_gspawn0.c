/* ISC license. */

#include <skalibs/cspawn.h>

#include <execline/config.h>
#include <execline/execline.h>

pid_t el_gspawn0 (char const *prog, char const *const *argv, char const *const *envp)
{
  if (!argv[0])
  {
    static char const *const newargv[3] = { EXECLINE_BINPREFIX "exit", "0", 0 } ;
    return gcspawn(newargv[0], newargv, envp, 0, 0, 0) ;
  }
  else return gcspawn(prog, argv, envp, 0, 0, 0) ;
}
