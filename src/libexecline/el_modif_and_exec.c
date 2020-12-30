/* ISC license. */

#include <string.h>

#include <skalibs/env.h>
#include <skalibs/exec.h>

#include <execline/config.h>
#include <execline/execline.h>

void el_modif_and_exec (char const *const *argv, char const *var, char const *value, int doimport)
{
  size_t varlen = strlen(var) ;
  size_t modiflen = value ? varlen + strlen(value) + 2 : 1 ;
  char modifs[modiflen] ;
  if (value)
  {
    memcpy(modifs, var, varlen) ;
    modifs[varlen] = '=' ;
    memcpy(modifs + varlen + 1, value, modiflen - varlen - 1) ;
  }
  if (doimport)
  {
    size_t m = 0 ;
    char const *newargv[env_len(argv) + 6] ;
    newargv[m++] = EXECLINE_BINPREFIX "importas" ;
    newargv[m++] = "-ui" ;
    newargv[m++] = "--" ;
    newargv[m++] = var ;
    newargv[m++] = var ;
    while (*argv) newargv[m++] = *argv++ ;
    newargv[m++] = 0 ;
    xmexec0_n(newargv, value ? modifs : var, value ? modiflen : varlen + 1, 1) ;
  }
  else xmexec0_n(argv, value ? modifs : var, value ? modiflen : varlen + 1, 1) ;
}
