/* ISC license. */

#include <skalibs/strerr2.h>
#include "exlsn.h"

#define USAGE "elgetpositionals [ -P num ] prog..."

int main (int argc, char const **argv, char const *const *envp)
{
  PROG = "elgetpositionals" ;
  exlsn_main(argc, argv, envp, &exlsn_exlp, USAGE) ;
}
