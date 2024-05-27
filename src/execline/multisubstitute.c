/* ISC license. */

#include <string.h>

#include <skalibs/strerr.h>

#include <execline/execline.h>
#include "exlsn.h"

#define USAGE "see http://skarnet.org/software/execline/multisubstitute.html"

int main (int argc, char const **argv, char const *const *envp)
{
  static char const *const commands[] =
  {
    "define",
    "importas",
    "elglob",
    "elgetpositionals",
    "multidefine",
    0
  } ;
  static exls_func_ref const functions[] =
  {
    &exlsn_define,
    &exlsn_importas,
    &exlsn_elglob,
    &exlsn_exlp,
    &exlsn_multidefine,
    0
  } ;
  exlsn_t info = EXLSN_ZERO ;
  int argc1 ;
  PROG = "multisubstitute" ;
  if (!--argc) strerr_dieusage(100, USAGE) ;

  argc1 = el_semicolon(++argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (argc1 + 1 == argc) strerr_dieusage(100, USAGE) ;

  while (argc1)
  {
    int n ;
    unsigned int i = 0 ;
    for (; commands[i] ; i++) if (!strcmp(*argv, commands[i])) break ;
    if (!commands[i]) strerr_dief3x(100, "syntax error: unrecognized", " directive ", *argv) ;
    n = (*(functions[i]))(argc1, argv, envp, &info) ;
    if (n < 0) switch (n)
    {
      case -3 : strerr_dief3x(100, "syntax error at", " directive ", commands[i]) ;
      case -2 : strerr_dief3x(100, "wrong key for", " directive ", commands[i]) ;
      case -1 : strerr_diefu3sys(111, "run", " directive ", commands[i]) ;
      default : strerr_dief3x(111, "unknown error with", " directive ", commands[i]) ;
    }
    argv += n ; argc1 -= n ; argc -= n ;
  }

  el_substandrun(argc-1, argv+1, envp, &info) ;
}
