/* ISC license. */

#include <execline/execline.h>

unsigned int el_semicolon_nostrict (char const **argv)
{
  unsigned int i = 0 ;
  for (; argv[i] ; i++)
  {
    if (!argv[i][0]) return i ;
    else if (argv[i][0] == ' ') argv[i]++ ;
  }
  return i + 1 ;
}
