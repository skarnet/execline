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
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/netstring.h>
#include <skalibs/ushort.h>
#include <execline/config.h>
#include <execline/execline.h>

#define USAGE "forbacktickx [ -p | -x breakcode,breakcode,... ] [ -n ] [ -C | -c ] [ -0 | -d delim ] var { backtickcmd... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

static int isbreak (unsigned short *tab, unsigned int n, int code)
{
  register unsigned int i = 0 ;
  for (; i < n ; i++) if ((unsigned short)code == tab[i]) break ;
  return i < n ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  genalloc pids = GENALLOC_ZERO ; /* pid_t */
  char const *delim = " \n\r\t" ;
  unsigned int delimlen = 4 ;
  char const *x ;
  int argc1 ;
  unsigned short breakcodes[256] ;
  unsigned int nbc = 0 ;
  int crunch = 0, chomp = 0 ;
  PROG = "forbacktickx" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "epnCc0d:x:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
	case 'e' : break ; /* compat */
        case 'p' :
        {
          if (!genalloc_ready(pid_t, &pids, 2))
            strerr_diefu1sys(111, "genalloc_ready") ;
          break ;
        }
        case 'n' : chomp = 1 ; break ;
        case 'C' : crunch = 1 ; break ;
        case 'c' : crunch = 0 ; break ;
        case '0' : delim = "" ; delimlen = 1 ; break ;
        case 'd' : delim = l.arg ; delimlen = str_len(delim) ; break ;
        case 'x' :
          if (!ushort_scanlist(breakcodes, 256, l.arg, &nbc)) dieusage() ;
          break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) dieusage() ;
  x = argv[0] ; if (!*x) dieusage() ;
  argv++ ; argc-- ;
  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  argv[argc1] = 0 ;
  {
    int fd ;
    pid_t pidw = el_spawn1(argv[0], argv, envp, &fd, 1) ;
    if (!pidw) strerr_diefu2sys(111, "spawn ", argv[0]) ;
    {
      char buf[BUFFER_INSIZE] ;
      buffer b = BUFFER_INIT(&buffer_read, fd, buf, BUFFER_INSIZE) ;
      stralloc modif = STRALLOC_ZERO ;
      unsigned int envlen = env_len(envp) ;
      unsigned int modifstart = str_len(x)+1 ;
      char const *newenv[envlen + 2] ;
      if (!stralloc_ready(&modif, modifstart+1))
        strerr_diefu1sys(111, "stralloc_ready") ;
      byte_copy(modif.s, modifstart-1, x) ;
      modif.s[modifstart-1] = '=' ;
      for (;;)
      {
        pid_t pid ;
        modif.len = modifstart ;
        if (delimlen)
        {
          register int r = skagetlnsep(&b, &modif, delim, delimlen) ;
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
          if (netstring_get(&b, &modif, &unread) <= 0)
          {
            if (netstring_okeof(&b, unread)) break ;
            else strerr_diefu1sys(111, "netstring_get") ;
          }
        }
        if (!stralloc_0(&modif)) strerr_diefu1sys(111, "stralloc_0") ;
        if (!env_merge(newenv, envlen+2, envp, envlen, modif.s, modif.len))
          strerr_diefu1sys(111, "merge environment") ;
        pid = el_spawn0(argv[argc1 + 1], argv + argc1 + 1, newenv) ;
        if (!pid) strerr_diefu2sys(111, "spawn ", argv[argc1+1]) ;
        if (pids.s)
        {
          if (!genalloc_append(pid_t, &pids, &pid))
            strerr_diefu1sys(111, "genalloc_append") ;
        }
        else
        {
          int wstat ;
          if (wait_pid(pid, &wstat) < 0)
            strerr_diefu2sys(111, "wait for ", argv[argc1 + 1]) ;
          if (isbreak(breakcodes, nbc, wait_status(wstat)))
            return wait_status(wstat) ;
        }
      }
      stralloc_free(&modif) ;
    }
    fd_close(fd) ;
    if (!genalloc_append(pid_t, &pids, &pidw))
      strerr_diefu1sys(111, "genalloc_append") ;
  }
  if (!waitn(genalloc_s(pid_t, &pids), genalloc_len(pid_t, &pids)))
    strerr_diefu1sys(111, "waitn") ;
  /* genalloc_free(pid_t, &pids) ; */
  return 0 ;
}
