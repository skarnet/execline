/* ISC license. */

#include <string.h>
#include <unistd.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>

#define USAGE "getpid variable prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  size_t len ;
  PROG = "getpid" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  len = strlen(argv[1]) ;
  if (!memchr(argv[1], '=', len))
    strerr_dief2x(100, "invalid variable name: ", argv[1]) ;
  {
    size_t i = len+1 ;
    char fmt[UINT_FMT + len + 2] ;
    memcpy(fmt, argv[1], len) ;
    fmt[len] = '=' ;
    i += uint_fmt(fmt+i, getpid()) ; fmt[i++] = 0 ;
    pathexec_r(argv+2, envp, env_len(envp), fmt, i) ;
  }
  strerr_dieexec(111, argv[2]) ;
}
