/* ISC license. */

#include <skalibs/strerr2.h>
#include "exlsn.h"

#define USAGE "define [ -N | -n ] [ -s ] [ -C | -c ] [ -d delim ] key value prog..."

int main (int argc, char const **argv, char const *const *envp)
{
  PROG = "define" ;
  exlsn_main(argc, argv, envp, &exlsn_define, USAGE) ;
}
