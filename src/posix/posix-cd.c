/* ISC license. */

#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>

#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/env.h>
#include <skalibs/exec.h>

#define USAGE "posix-cd [ -L | -P ] [ - | path ] [ prog... ]"
#define dieusage() strerr_dieusage(100, USAGE)
#define dienomem() strerr_diefu1sys(111, "stralloc_catb")

int main (int argc, char const **argv)
{
  int phy = 0 ;
  int dopwd = 0 ;
  char const *where ;
  int got = 0 ;
  stralloc sa = STRALLOC_ZERO ;
  PROG = "posix-cd" ;
  setlocale(LC_ALL, "") ; /* yeah, as if */

  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "LP", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'L' : phy = 0 ; break ;
        case 'P' : phy = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) where = getenv("HOME") ;
  else
  {
    where = *argv++ ;
    if (!strcmp(where, "-"))
    {
      where = getenv("OLDPWD") ;
      dopwd = 1 ;
    }
  }
  if (!where || !where[0]) dieusage() ;

  if (!(where[0] == '/' || (where[0] == '.' && (!where[1] || where[1] == '/' || (where[1] == '.' && (!where[2] || where[2] == '/'))))))
  {
    char const *cdpath = getenv("CDPATH") ;
    if (cdpath)
    {
      size_t pos = 0 ;
      size_t len = strlen(cdpath) ;
      while (pos < len)
      {
        struct stat st ;
        size_t m = byte_chr(cdpath + pos, len - pos, ':') ;
        sa.len = 0 ;
        if (m)
        {
          if (!stralloc_catb(&sa, cdpath + pos, m)) dienomem() ;
          if (cdpath[pos + m - 1] != '/' && !stralloc_catb(&sa, "/", 1)) dienomem() ;
        }
        else if (!stralloc_catb(&sa, "./", 2)) dienomem() ;
        if (!stralloc_cats(&sa, where) || !stralloc_0(&sa)) dienomem() ;
        if (!stat(sa.s, &st) && S_ISDIR(st.st_mode))
        {
          got = 1 ;
          dopwd = 1 ;
          break ;
        }
        pos += m+1 ;
      }
    }
  }

  if (!got && (!stralloc_cats(&sa, where) || !stralloc_0(&sa))) dienomem() ;

  {
    size_t sabase = sa.len ;
    if (sagetcwd(&sa) < 0) strerr_diefu1sys(111, "getcwd") ;
    if (!stralloc_0(&sa)) dienomem() ;
    if (!env_mexec("OLDPWD", sa.s + sabase)) dienomem() ;
    sa.len = sabase ;
  }

  if (!phy)
  {
    char const *x = getenv("PWD") ;
    if (x && sa.s[0] != '/')
    {
      size_t len = strlen(x) ;
      int doslash = len && x[len-1] != '/' ;
      if (!stralloc_insertb(&sa, 0, x, len + doslash)) dienomem() ;
      if (doslash) sa.s[len] = '/' ;
    }
    {
      stralloc tmp = STRALLOC_ZERO ;
      if (!stralloc_ready(&tmp, sa.len + 2)) dienomem() ;
      tmp.len = path_canonicalize(tmp.s, sa.s, 1) ;
      if (!tmp.len++)
        strerr_diefu4sys(111, "canonicalize ", sa.s, ": problem with ", tmp.s) ;
      stralloc_free(&sa) ;
      sa = tmp ;
    }
    if (!env_mexec("PWD", sa.s)) dienomem() ;
#ifdef PATH_MAX
    if (sa.len > PATH_MAX && strlen(where) < PATH_MAX && x && *x)
    {
      size_t len = strlen(x) ;
      int hasslash = x[len-1] == '/' ;
      if (!strncmp(sa.s, x, len))
      {
        if (hasslash || (sa.len > len && sa.s[len] == '/'))
        {
          sa.len -= len + !hasslash ;
          memmove(sa.s, sa.s + len + !hasslash, sa.len) ;
        }
      }
    }
#endif
  }

 /* fking finally */

  if (chdir(sa.s) < 0)
    strerr_diefu2sys(111, "chdir to ", where) ;

 /* and there's still more nonsense to do afterwards! */

  if (phy)
  {
    sa.len = 0 ;
    if (sagetcwd(&sa) < 0) strerr_diefu1sys(111, "getcwd") ;
    if (!stralloc_0(&sa)) dienomem() ;
    if (!env_mexec("PWD", sa.s)) dienomem() ;
  }
  
  if (dopwd)
  {
    sa.s[sa.len - 1] = '\n' ;
    if (allwrite(1, sa.s, sa.len) < sa.len)
      strerr_diefu1sys(111, "write to stdout") ;
  }

  xmexec0(argv) ;
}
