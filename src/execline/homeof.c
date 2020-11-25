/* ISC license. */

#include <pwd.h>
#include <errno.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>

#define USAGE "homeof user"

int main (int argc, char const *const *argv)
{
  struct passwd *pw ;
  PROG = "homeof" ;
  if (argc < 2) strerr_dieusage(100, USAGE) ;
  pw = getpwnam(argv[1]) ;
  if (!pw)
  {
    if (errno)
      strerr_diefu2sys(111, "get passwd entry for ", argv[1]) ;
    else
      strerr_diefu3x(111, "get passwd entry for ", argv[1], ": no such user") ;
  }
  if ((buffer_puts(buffer_1small, pw->pw_dir) < 0)
   || (buffer_putflush(buffer_1small, "\n", 1) < 0))
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
