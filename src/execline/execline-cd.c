/* ISC license. */

#include <unistd.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE "cd path prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  PROG = "execline-cd" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  if (chdir(argv[1]) == -1)
    strerr_diefu2sys(111, "chdir to ", argv[1]) ;
  xpathexec_run(argv[2], argv+2, envp) ;
}
