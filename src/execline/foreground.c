/* ISC license. */

#include <skalibs/strerr2.h>
#include <execline/execline.h>

int main (int argc, char const **argv, char const *const *envp)
{
  int argc1 ;
  PROG = "foreground" ;
  argc1 = el_semicolon(++argv) ;
  if (argc1 >= --argc) strerr_dief1x(100, "unterminated block") ;
  argv[argc1] = 0 ;
  el_execsequence(argv, argv+argc1+1, envp) ;
}
