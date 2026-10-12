/* ISC license. */

#include <unistd.h>

#include <skalibs/types.h>
#include <skalibs/buffer.h>
#include <skalibs/prog.h>
#include <skalibs/strerr.h>

#define USAGE "dollarsharp [ args ]"

int main (int argc, char const *const *argv)
{
  size_t len ;
  char fmt[UINT_FMT] ;
  PROG = "dollarsharp" ;
  if (argc-- <= 0) strerr_dieusage(100, USAGE) ;
  len = uint_fmt(fmt, argc) ;
  fmt[len++] = '\n' ;
  if (buffer_putflush(buffer_1small, fmt, len) == -1)
    strerr_diefu1sys(111, "write to stdout") ;
  _exit(0) ;
}
