/* ISC license. */

#include <skalibs/strerr2.h>
#include "exlsn.h"

#define USAGE "importas [ -i | -D default ] [ -u ] [ -N | -n ] [ -s ] [ -C | -c ] [ -d delim ] key var prog..."

int main (int argc, char const **argv, char const *const *envp)
{
  PROG = "importas" ;
  exlsn_main(argc, argv, envp, &exlsn_importas, USAGE) ;
}
