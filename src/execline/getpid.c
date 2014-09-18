/* ISC license. */

#include <unistd.h>
#include <skalibs/bytestr.h>
#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>

#define USAGE "getpid variable prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int len ;
  PROG = "getpid" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  len = str_len(argv[1]) ;
  if (byte_chr(argv[1], len, '=') < len)
    strerr_dief2x(100, "invalid variable name: ", argv[1]) ;
  {
    char fmt[UINT_FMT + len + 2] ;
    unsigned int i = len+1 ;
    byte_copy(fmt, len, argv[1]) ;
    fmt[len] = '=' ;
    i += uint_fmt(fmt+i, getpid()) ; fmt[i++] = 0 ;
    pathexec_r(argv+2, envp, env_len(envp), fmt, i) ;
  }
  strerr_dieexec(111, argv[2]) ;
}
