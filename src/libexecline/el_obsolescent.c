/* ISC license. */

#include <skalibs/strerr2.h>
#include <execline/execline.h>

void el_obsolescent (void)
{
  if (el_getstrict())
    strerr_warnw3x("this command is marked as obsolescent. Please update your script to use the ", PROG, "x command instead.") ;
}
