/* ISC license. */

#include <skalibs/bytestr.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#define USAGE "getcwd variable prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  stralloc sa = STRALLOC_ZERO ;
  PROG = "getcwd" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  if (argv[1][str_chr(argv[1], '=')])
    strerr_dief2x(100, "invalid variable name: ", argv[1]) ;
  if (!stralloc_cats(&sa, argv[1]) || !stralloc_catb(&sa, "=", 1))
    strerr_diefu1sys(111, "stralloc_catb") ;
  if (sagetcwd(&sa) < 0) strerr_diefu1sys(111, "getcwd") ;
  if (!stralloc_0(&sa)) strerr_diefu1sys(111, "stralloc_catb") ;
  pathexec_r(argv + 2, envp, env_len(envp), sa.s, sa.len) ;
  strerr_dieexec(111, argv[2]) ;
}
