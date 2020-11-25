/* ISC license. */

#include <string.h>

#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#define USAGE "exec [ -c ] [ -l ] [ -a argv0 ] prog..."

int main (int argc, char const **argv, char const *const *envp)
{
  static char const *const zero = 0 ;
  char const *executable = 0 ;
  char const *argv0 = 0 ;
  int dash = 0 ;
  PROG = "exec" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "cla:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'c' : envp = &zero ; break ;
        case 'l' : dash = 1 ; break ;
        case 'a' : argv0 = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;

  executable = argv[0] ;
  if (argv0) argv[0] = argv0 ;
  if (dash)
  {
    size_t n = strlen(argv[0]) ;
    char dashed[n+2] ;
    dashed[0] = '-' ;
    memcpy(dashed+1, argv[0], n+1) ;
    argv[0] = (char const *)dashed ;
    xexec_ae(executable, argv, envp) ;
  }
  else xexec_ae(executable, argv, envp) ;
}
