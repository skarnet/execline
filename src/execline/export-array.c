/* ISC license. */

#include <string.h>

#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/netstring.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "export-array [ -d delim ] variable { value-list } prog..."
#define dieusage() strerr_dieusage(100, USAGE)
#define dienomem() strerr_diefu1sys(111, "stralloc_catb")

int main (int argc, char const **argv)
{
  stralloc sa = STRALLOC_ZERO ;
  char const *var ;
  int argc1 ;
  char delim = 0 ;
  PROG = "export-array" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "d", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'd' : delim = *l.arg ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) dieusage() ;
  var = *argv++ ; argc-- ;
  if (strchr(var, '=')) strerr_dief2x(100, "invalid variable name: ", var) ;
  
  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (argc1 + 1 == argc) dieusage() ;

  if (!stralloc_cats(&sa, var) || !stralloc_catb(&sa, "=", 1)) dienomem() ;
  for (; argc1 ; argv++, argc1--)
  {
    if (delim ? !stralloc_cats(&sa, *argv) || !stralloc_catb(&sa, &delim, 1) : !netstring_appends(&sa, *argv)) dienomem() ;
  }
  if (!stralloc_0(&sa)) dienomem() ;

  xmexec_n(argv + 1, sa.s, sa.len, 1) ;
}
