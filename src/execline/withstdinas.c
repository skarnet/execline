/* ISC license. */

#include <string.h>

#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#include <execline/execline.h>

#define USAGE "withstdinas [ -i | -I | -D default ] [ -N | -n ] [ -E | -e ] var remainder..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  subgetopt_t localopt = SUBGETOPT_ZERO ;
  stralloc value = STRALLOC_ZERO ;
  int insist = 0, chomp = 1, doimport = 0 ;
  char const *def = 0 ;
  char const *val ;
  PROG = "withstdinas" ;
  for (;;)
  {
    int opt = subgetopt_r(argc, argv, "iINnD:Ee", &localopt) ;
    if (opt < 0) break ;
    switch (opt)
    {
      case 'i' : insist = 2 ; break ;
      case 'I' : insist = 1 ; break ;
      case 'N' : chomp = 0 ; break ;
      case 'n' : chomp = 1 ; break ;
      case 'D' : def = localopt.arg ; break ;
      case 'E' : doimport = 1 ; break ;
      case 'e' : doimport = 0 ; break ;
      default : dieusage() ;
    }
  }
  argc -= localopt.ind ; argv += localopt.ind ;
  if (!argc) dieusage() ;
  if (!argv[0][0] || strchr(argv[0], '=')) strerr_dief1x(100, "invalid variable name") ;

  if (!slurp(&value, 0) || !stralloc_0(&value)) strerr_diefu1sys(111, "slurp") ;
  val = value.s ;
  if (strlen(value.s) < value.len - 1)
  {
    if (insist >= 2)
      strerr_dief1x(1, "stdin contained a null character") ;
    else if (insist) val = 0 ;
    else if (def)
    {
      val = def ;
      strerr_warnw2x("stdin contained a null character", " - using default instead") ;
    }
  }
  else if (chomp && (value.s[value.len - 2] == '\n'))
    value.s[--value.len - 1] = 0 ;
  el_modif_and_exec(argv + 1, argv[0], val, doimport) ;
}
