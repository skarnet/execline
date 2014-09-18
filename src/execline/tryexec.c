/* ISC license. */

#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

#define USAGE "tryexec [ -n ] [ -c ] [ -l ] [ -a argv0 ] { command... }"

int main (int argc, char const **argv, char const *const *envp)
{
  char const *env_zero[1] = { 0 } ;
  char const *executable = 0 ;
  char const *argv0 = 0 ;
  char const **dom = 0 ;
  char const **sub = 0 ;
  char const *const *dom_envp = envp ;
  int not = 0, dash = 0 ;
  PROG = "tryexec" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "ncla:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : not = 1 ; break ;
        case 'c' : dom_envp = env_zero ; break ;
        case 'l' : dash = 1 ; break ;
        case 'a' : argv0 = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  {
    int argc1 = el_semicolon(argv) ;
    if (!argc1) strerr_dief1x(100, "empty block") ;
    if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
    argv[argc1] = 0 ;
    dom = argv + not * (argc1 + 1) ;
    sub = argv + !not * (argc1 + 1) ;
  }
  executable = dom[0] ;
  if (argv0) dom[0] = argv0 ;
  if (dash)
  {
    register unsigned int n = str_len(dom[0]) ;
    char dashed[n+2] ;
    dashed[0] = '-' ;
    byte_copy(dashed+1, n+1, dom[0]) ;
    dom[0] = dashed ;
    pathexec_run(executable, dom, dom_envp) ;
  }
  else pathexec_run(executable, dom, dom_envp) ;

  pathexec0_run(sub, envp) ;
  strerr_dieexec(111, sub[0]) ;
}
