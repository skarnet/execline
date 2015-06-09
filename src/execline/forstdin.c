/* ISC license. */

#include <sys/types.h>
#include <errno.h>
#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <skalibs/fmtscan.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/env.h>
#include <skalibs/sig.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/netstring.h>
#include <skalibs/ushort.h>
#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forstdin [ -p | -o okcode,okcode,... | -x breakcode,breakcode,... ] [ -n ] [ -C | -c ] [ -0 | -d delim ] var command..."
#define dieusage() strerr_dieusage(100, USAGE)

static genalloc pids = GENALLOC_ZERO ; /* pid_t */

static int isok (unsigned short *tab, unsigned int n, int code)
{
  register unsigned int i = 0 ;
  for (; i < n ; i++) if ((unsigned short)code == tab[i]) break ;
  return i < n ;
}

static void parallel_sigchld_handler (int sig)
{
  pid_t *tab = genalloc_s(pid_t, &pids) ;
  unsigned int len = genalloc_len(pid_t, &pids) ;
  int wstat ;
  for (;;)
  {
    register int r = wait_pids_nohang(tab, len, &wstat) ;
    if (r <= 0) break ;
    tab[r-1] = tab[--len] ;
  }
  genalloc_setlen(pid_t, &pids, len) ;
  (void)sig ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  char const *delim = " \n\r\t" ;
  unsigned int delimlen = 4 ;
  unsigned short okcodes[256] ;
  unsigned int nbc = 0 ;
  int crunch = 0, chomp = 0, not = 1 ;
  PROG = "forstdin" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "pnCc0d:o:x:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p' :
        {
          if (!genalloc_ready(pid_t, &pids, 1))
            strerr_diefu1sys(111, "genalloc_ready") ;
          break ;
        }
        case 'n' : chomp = 1 ; break ;
        case 'C' : crunch = 1 ; break ;
        case 'c' : crunch = 0 ; break ;
        case '0' : delim = "" ; delimlen = 1 ; break ;
        case 'd' : delim = l.arg ; delimlen = str_len(delim) ; break ;
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
    unsigned int envlen = env_len(envp) ;
    unsigned int modifstart = str_len(argv[0])+1 ;
    char const *newenv[envlen + 2] ;
    if (!stralloc_ready(&modif, modifstart+1))
      strerr_diefu1sys(111, "stralloc_ready") ;
    byte_copy(modif.s, modifstart-1, argv[0]) ;
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
        register int r = skagetlnsep(buffer_0, &modif, delim, delimlen) ;
        if (!r) break ;
        else if (r < 0)
        {
          if (errno != EPIPE) strerr_diefu1sys(111, "skagetlnsep") ;
          if (chomp) break ;
        }
        else modif.len-- ;
        if ((modif.len == modifstart) && crunch) continue ;
      }
      else
      {
        unsigned int unread = 0 ;
        if (netstring_get(buffer_0, &modif, &unread) <= 0)
        {
          if (netstring_okeof(buffer_0, unread)) break ;
          else strerr_diefu1sys(111, "netstring_get") ;
        }
      }
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
  return 0 ;
}
