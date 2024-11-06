/* ISC license. */

#include <execline/config.h>
#include <execline/execline.h>

static char const *const el_trueargv_[3] = { EXECLINE_BINPREFIX "exit", "0", 0 } ;
char const *const *el_trueargv = el_trueargv_ ;
