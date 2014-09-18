/* ISC license. */

#include <skalibs/bytestr.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>

#define USAGE "export variable value prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int len1 ;
  PROG = "export" ;
  if (argc < 4) strerr_dieusage(100, USAGE) ;
  len1 = str_len(argv[1]) ;
  if (byte_chr(argv[1], len1, '=') < len1)
    strerr_dief2x(100, "invalid variable name: ", argv[1]) ;
  {
    unsigned int len2 = str_len(argv[2]) ;
    char fmt[len1 + len2 + 2] ;
    byte_copy(fmt, len1, argv[1]) ;
    fmt[len1] = '=' ;
    byte_copy(fmt + len1 + 1, len2 + 1, argv[2]) ;
    pathexec_r(argv+3, envp, env_len(envp), fmt, len1 + len2 + 2) ;
  }
  strerr_dieexec(111, argv[3]) ;
}
