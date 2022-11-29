/* ISC license. */

#include <string.h>
#include <unistd.h>

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>

#include <execline/execline.h>

#define USAGE "getpid [ -E | -e ] [ -P | -p ] variable prog..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  int doimport = 0 ;
  int doppid = 0 ;
  char fmt[PID_FMT] ;
  PROG = "getpid" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "EePp", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'E' : doimport = 1 ; break ;
        case 'e' : doimport = 0 ; break ;
        case 'P' : doppid = 1 ; break ;
        case 'p' : doppid = 0 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) dieusage() ;
  if (!argv[0][0] || strchr(argv[0], '=')) strerr_dief1x(100, "invalid variable name") ;

  fmt[pid_fmt(fmt, doppid ? getppid() : getpid())] = 0 ;
  el_modif_and_exec(argv + 1, argv[0], fmt, doimport) ;
}
