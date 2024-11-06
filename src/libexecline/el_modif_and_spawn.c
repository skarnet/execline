/* ISC license. */

#include <string.h>

#include <skalibs/posixplz.h>
#include <skalibs/env.h>
#include <skalibs/cspawn.h>

#include <execline/config.h>
#include <execline/execline.h>

pid_t el_modif_and_spawn (char const *const *argv, char const *var, char const *value, int doimport)
{
  size_t varlen = strlen(var) ;
  size_t modiflen = value ? varlen + strlen(value) + 2 : 1 ;
  size_t envlen = env_len((char const *const *)environ) ;
  char const *newenv[envlen + 2] ;
  char modifs[modiflen] ;
  if (value)
  {
    memcpy(modifs, var, varlen) ;
    modifs[varlen] = '=' ;
    memcpy(modifs + varlen + 1, value, modiflen - varlen - 1) ;
  }
  env_mergen(newenv, envlen + 2, (char const *const *)environ, envlen, value ? modifs : var, value ? modiflen : varlen + 1, 1) ;
  if (doimport && argv[0])
  {
    size_t m = 0 ;
    char const *newargv[env_len(argv) + 5] ;
    newargv[m++] = EXECLINE_BINPREFIX "importas" ;
    newargv[m++] = "-uSi" ;
    newargv[m++] = "--" ;
    newargv[m++] = var ;
    while (*argv) newargv[m++] = *argv++ ;
    newargv[m++] = 0 ;
    return cspawn(newargv[0], newargv, newenv, CSPAWN_FLAGS_SIGBLOCKNONE, 0, 0) ;
  }
  if (!argv[0]) argv = el_trueargv ;
  return cspawn(argv[0], argv, newenv, CSPAWN_FLAGS_SIGBLOCKNONE, 0, 0) ;
}
