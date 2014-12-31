/* ISC license. */

#ifndef EXLSN_H
#define EXLSN_H

#include <skalibs/gccattributes.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>

typedef struct exlsn_s exlsn_t, *exlsn_t_ref ;
struct exlsn_s
{
  stralloc vars ;
  stralloc values ;
  genalloc data ; /* array of elsubst */
  stralloc modifs ;
} ;

#define EXLSN_ZERO { .vars = STRALLOC_ZERO, .values = STRALLOC_ZERO, .data = GENALLOC_ZERO, .modifs = STRALLOC_ZERO }

extern void exlsn_free (exlsn_t *) ;

typedef int exlsnfunc_t (int, char const **, char const *const *, exlsn_t *) ;
typedef exlsnfunc_t *exlsnfunc_t_ref ;
	
extern exlsnfunc_t exlsn_define ;
extern exlsnfunc_t exlsn_importas ;
extern exlsnfunc_t exlsn_import ;
extern exlsnfunc_t exlsn_elglob ;
extern exlsnfunc_t exlsn_exlp ;
extern exlsnfunc_t exlsn_multidefine ;

extern int exlp (unsigned int, char const *const *, exlsn_t *) ;
extern void el_substandrun (int, char const *const *, char const *const *, exlsn_t const *) gccattr_noreturn ;
extern void el_substandrun_str (stralloc *, unsigned int, char const *const *, exlsn_t const *) gccattr_noreturn ;
extern void exlsn_main (int, char const **, char const *const *, exlsnfunc_t *, char const *) gccattr_noreturn ;

#endif
