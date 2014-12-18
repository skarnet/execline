/* ISC license. */

#include <sys/types.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

pid_t el_spawn1 (char const *prog, char const *const *argv, char const *const *envp, int *fd, int w)
{
  if (!argv[0])
  {
    static char const *const newargv[2] = { "/bin/true", 0 } ;
    return child_spawn1_pipe(newargv[0], newargv, 0, fd, w) ;
  }
  else return child_spawn1_pipe(prog, argv, envp, fd, w) ;
}
