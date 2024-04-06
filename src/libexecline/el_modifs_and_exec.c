/* ISC license. */

#include <string.h>

#include <skalibs/env.h>
#include <skalibs/exec.h>

#include <execline/config.h>
#include <execline/execline.h>

void el_modifs_and_exec (char const *const *argv, char const *const *vars, char const *const *values, size_t n, int doimport)
{
  size_t pos = 0 ;
  size_t yeslen = 0 ;
  size_t yesn = 0 ;
  size_t modiflen = 0 ;
  for (size_t i = 0 ; i < n ; i++)
  {
    size_t len = strlen(vars[i]) + 1 ;
    modiflen += len ;
    if (values[i])
    {
      yesn++ ;
      yeslen += len ;
      modiflen += 1 + strlen(values[i]) ;
    }
  }

  char modifs[modiflen ? modiflen : 1] ;

  for (size_t i = 0 ; i < n ; i++)
  {
    size_t len = strlen(vars[i]) ;
    memcpy(modifs + pos, vars[i], len) ;
    pos += len ;
    if (values[i])
    {
      modifs[pos++] = '=' ;
      len = strlen(values[i]) ;
      memcpy(modifs + pos, values[i], len) ;
      pos += len ;
    }
    modifs[pos++] = 0 ;
  }

  if (doimport && yesn)
  {
    size_t m = 0 ;
    size_t ypos = 0 ;
    char const *newargv[env_len(argv) + 3 + 5 * yesn] ;
    char yesvars[yeslen ? yeslen : 1] ;
    newargv[m++] = EXECLINE_BINPREFIX "multisubstitute" ;
    for (size_t i = 0 ; i < n ; i++) if (values[i])
    {
      size_t len = strlen(vars[i]) + 1 ;
      char *p = yesvars + ypos ;
      newargv[m++] = " importas" ;
      newargv[m++] = " -ui" ;
      newargv[m++] = " --" ;
      newargv[m++] = p ;
      newargv[m++] = p ;
      yesvars[ypos++] = ' ' ;
      memcpy(yesvars + ypos, vars[i], len) ;
      ypos += len ;
    }
    newargv[m++] = "" ;
    while (*argv) newargv[m++] = *argv++ ;
    newargv[m++] = 0 ;
    xmexec0_n(newargv, modifs, modiflen, n) ;
  }
  else xmexec0_n(argv, modifs, modiflen, n) ;
}
