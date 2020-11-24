/* ISC license. */

#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/env.h>
#include <skalibs/sig.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/netstring.h>

#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forstdin [ -p | -o okcode,okcode,... | -x breakcode,breakcode,... ] [ -N | -n ] [ -C | -c ] [ -0 | -d delim ] var command..."
#define dieusage() strerr_dieusage(100, USAGE)

static genalloc pids = GENALLOC_ZERO ; /* pid_t */

static int isok (unsigned short *tab, unsigned int n, int code)
{
  unsigned int i = 0 ;
  for (; i < n ; i++) if ((unsigned short)code == tab[i]) break ;
  return i < n ;
}

static void parallel_sigchld_handler (int sig)
{
  pid_t *tab = genalloc_s(pid_t, &pids) ;
  size_t len = genalloc_len(pid_t, &pids) ;
  int wstat ;
  for (;;)
  {
    ssize_t r = wait_pids_nohang(tab, len, &wstat) ;
    if (r <= 0) break ;
    tab[r-1] = tab[--len] ;
  }
  genalloc_setlen(pid_t, &pids, len) ;
  (void)sig ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  char const *delim = "\n" ;
  size_t delimlen = 1 ;
  size_t nbc = 0 ;
  unsigned short okcodes[256] ;
  int crunch = 0, chomp = 1, not = 1, eofcode = 1 ;
  PROG = "forstdin" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "pNnCc0d:o:x:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' :
        {
          if (!genalloc_ready(pid_t, &pids, 1))
            strerr_diefu1sys(111, "genalloc_ready") ;
          break ;
        }
        case 'N' : chomp = 0 ; break ;
        case 'n' : chomp = 1 ; break ;
        case 'C' : crunch = 1 ; break ;
        case 'c' : crunch = 0 ; break ;
        case '0' : delim = "" ; delimlen = 1 ; break ;
        case 'd' : delim = l.arg ; delimlen = strlen(delim) ; break ;
        case 'o' :
          not = 0 ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        case 'x' :
          not = 1 ;
          if (!ushort_scanlist(okcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) dieusage() ;
  {
    stralloc modif = STRALLOC_ZERO ;
    size_t envlen = env_len(envp) ;
    size_t modifstart = strlen(argv[0]) + 1 ;
    char const *newenv[envlen + 2] ;
    if (!stralloc_ready(&modif, modifstart+1))
      strerr_diefu1sys(111, "stralloc_ready") ;
    memcpy(modif.s, argv[0], modifstart - 1) ;
    modif.s[modifstart-1] = '=' ;
    if (pids.s)
    {
      if (sig_catch(SIGCHLD, &parallel_sigchld_handler) < 0)
        strerr_diefu1sys(111, "install SIGCHLD handler") ;
    }
    for (;;)
    {
      pid_t pid ;
      modif.len = modifstart ;
      if (delimlen)
      {
        int r = skagetlnsep(buffer_0, &modif, delim, delimlen) ;
        if (!r) break ;
        else if (r < 0)
        {
          if (errno != EPIPE) strerr_diefu1sys(111, "skagetlnsep") ;
          if (chomp) break ;
        }
        if (crunch && modif.len == modifstart + 1) continue ;
        if (chomp) modif.len-- ;
      }
      else
      {
        size_t unread = 0 ;
        if (netstring_get(buffer_0, &modif, &unread) <= 0)
        {
          if (netstring_okeof(buffer_0, unread)) break ;
          else strerr_diefu1sys(111, "netstring_get") ;
        }
      }
      eofcode = 0 ;
      if (!stralloc_0(&modif)) strerr_diefu1sys(111, "stralloc_0") ;
      if (!env_merge(newenv, envlen+2, envp, envlen, modif.s, modif.len))
        strerr_diefu1sys(111, "merge environment") ;
      if (pids.s) sig_block(SIGCHLD) ;
      pid = el_spawn0(argv[1], argv + 1, newenv) ;
      if (!pid) strerr_diefu2sys(111, "spawn ", argv[1]) ;
      if (pids.s)
      {
        if (!genalloc_append(pid_t, &pids, &pid))
          strerr_diefu1sys(111, "genalloc_append") ;
        sig_unblock(SIGCHLD) ;
      }
      else
      {
        int wstat ;
        if (wait_pid(pid, &wstat) < 0)
          strerr_diefu2sys(111, "wait for ", argv[1]) ;
        if (not == isok(okcodes, nbc, wait_estatus(wstat)))
          return wait_estatus(wstat) ;
      }
    }
    stralloc_free(&modif) ;
  }
  if (pids.s)
    for (;;)
    {
      sig_block(SIGCHLD) ;
      if (!pids.len) break ;
      sig_pause() ;
    }
  return eofcode ;
}
