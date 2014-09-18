/* ISC license. */

#include <skalibs/strerr2.h>
#include "exlsn.h"

#define USAGE "elglob [ -v ] [ -w ] [ -s ] [ -m ] [ -e ] [ -0 ] key pattern prog..."

int main (int argc, char const **argv, char const *const *envp)
{
  PROG = "elglob" ;
  exlsn_main(argc, argv, envp, &exlsn_elglob, USAGE) ;
}
