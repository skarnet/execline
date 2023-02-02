#!/bin/sh -e

echo '/* ISC license. */'
echo
echo '#include <skalibs/nonposix.h>'
echo
{ echo '#include <string.h>' ; echo '#include <stdlib.h>' ; cat src/execline/*.c | grep '^#include <' | grep -vF '<skalibs/' | grep -vF '<execline/' ; } | sort -u

cat <<EOF

#include <skalibs/nsig.h>
#include <skalibs/skalibs.h>

#include <execline/config.h>
#include <execline/execline.h>
#include "exlsn.h"

typedef int main_func (int, char **, char const *const *) ;
typedef main_func *main_func_ref ;

typedef struct execline_app_s execline_app, *execline_app_ref ;
struct execline_app_s
{
  char const *name ;
  main_func_ref mainf ;
} ;

static int execline_app_cmp (void const *a, void const *b)
{
  char const *name = a ;
  execline_app const *p = b ;
  return strcmp(name, p->name) ;
}

#ifdef EXECLINE_PEDANTIC_POSIX
# define CD_FUNC posix_cd_main
# define UMASK_FUNC posix_umask_main
#else
# define CD_FUNC execline_cd_main
# define UMASK_FUNC execline_umask_main
#endif

EOF

for i in `ls -1 src/execline/deps-exe` ; do
  j=`echo $i | tr - _`
  echo
  grep -v '^#include ' < src/execline/${i}.c | grep -vF '/* ISC license. */' | sed -e "s/int main (int argc, char \(.*\)\*argv.*$/int ${j}_main (int argc, char \1*argv, char const *const *envp)/"
  echo
  echo '#undef USAGE'
  echo '#undef dieusage'
  echo '#undef dienomem'
done

cat <<EOF

static int execline_main (int, char **, char const *const *) ;

static execline_app const execline_apps[] =
{
EOF

for i in `{ echo cd ; echo execline ; echo umask ; ls -1 src/execline/deps-exe ; } | sort` ; do
  j=`echo $i | tr - _`
  if test $i = cd ; then
    echo '  { .name = "cd", .mainf = (main_func_ref)&CD_FUNC },'
  elif test $i = umask ; then
    echo '  { .name = "umask", .mainf = (main_func_ref)&UMASK_FUNC },'
  else
    echo "  { .name=\"${i}\", .mainf = (main_func_ref)&${j}_main },"
  fi
done

cat <<EOF
} ;

#define USAGE "execline subcommand [ arguments... ]"
#define dieusage() strerr_dief1x(100, USAGE)

static int execline_main (int argc, char **argv, char const *const *envp)
{
  execline_app const *p ;
  PROG = "execline" ;
  if (!argc) dieusage() ;
  p = bsearch(argv[1], execline_apps, sizeof(execline_apps) / sizeof(execline_app), sizeof(execline_app), &execline_app_cmp) ;
  if (!p) strerr_dief2x(100, "unknown subcommand: ", argv[1]) ;
  return (*(p->mainf))(argc-1, argv+1, envp) ;
}

int main (int argc, char **argv, char const *const *envp)
{
  execline_app const *p ;
  char const *name = strrchr(argv[0], '/') ;
  if (name) name++ ; else name = argv[0] ;
  p = bsearch(name, execline_apps, sizeof(execline_apps) / sizeof(execline_app), sizeof(execline_app), &execline_app_cmp) ;
  return p ? (*(p->mainf))(argc, argv, envp) : execline_main(argc, argv, envp) ;
}
EOF
