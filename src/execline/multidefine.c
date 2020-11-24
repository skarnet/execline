/* ISC license. */

#include <skalibs/strerr2.h>
#include "exlsn.h"

#define USAGE "multidefine [ -0 ] [ -r ] [ -N | -n ] [ -C | -c ] [ -d delim ] value { vars... } prog..."

int main (int argc, char const **argv, char const *const *envp)
{
  PROG = "multidefine" ;
  exlsn_main(argc, argv, envp, &exlsn_multidefine, USAGE) ;
}
