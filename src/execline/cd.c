/* ISC license. */

#include <unistd.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE "cd path prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  PROG = "cd" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  if (chdir(argv[1]) == -1)
    strerr_diefu2sys(111, "chdir to ", argv[1]) ;
  pathexec_run(argv[2], argv+2, envp) ;
  strerr_dieexec(111, argv[2]) ;
}
