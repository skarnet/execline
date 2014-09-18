/* ISC license. */

#include <skalibs/bytestr.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>

#define USAGE "unexport variable prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int len ;
  PROG = "unexport" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  len = str_len(argv[1]) ;
  if (byte_chr(argv[1], len, '=') < len)
    strerr_dief2x(100, "invalid variable name: ", argv[1]) ;
  pathexec_r(argv+2, envp, env_len(envp), argv[1], len+1) ;
  strerr_dieexec(111, argv[2]) ;
}
