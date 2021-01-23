/* ISC license. */

#include <skalibs/nonposix.h>  /* for SKALIBS_NSIG to work */
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/nsig.h>
#include <skalibs/sig.h>
#include <skalibs/strerr2.h>
#include <skalibs/iopause.h>
#include <skalibs/selfpipe.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>

#include <execline/execline.h>

#define USAGE "trap [ -x ] { signal { cmdline } ... } prog..."
#define dieusage() strerr_dieusage(100, USAGE) ;

static pid_t pids[SKALIBS_NSIG + 1] ;
static char const *const *argvs[SKALIBS_NSIG] ; /* initted with 0s */

static inline void action (unsigned int i, char const *const *envp, size_t envlen)
{
  if (argvs[i])
  {
    if (!pids[i])
    {
      char const *newenvp[envlen + 3] ;
      char modif[9 + PID_FMT + UINT_FMT] = "!=" ;
      size_t m = 2 ;
      m += pid_fmt(modif + m, pids[SKALIBS_NSIG]) ;
      modif[m++] = 0 ;
      memcpy(modif + m, "SIGNAL=", 7) ; m += 7 ;
      m += uint_fmt(modif + m, i) ;
      modif[m++] = 0 ;
      if (!env_mergen(newenvp, envlen + 3, envp, envlen, modif, m, 2))
        strerr_diefu1sys(111, "adjust environment for child") ;
      pids[i] = child_spawn0(argvs[i][0], argvs[i], newenvp) ;
      if (!pids[i]) strerr_diefu2sys(111, "spawn ", argvs[i][0]) ;
    }
  }
  else kill(pids[SKALIBS_NSIG], i) ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  size_t envlen = env_len(envp) ;
  iopause_fd x = { .events = IOPAUSE_READ } ;
  sigset_t full, set ;
  int xfersigs = 0 ;
  unsigned int argc1 ;
  unsigned int i = 0 ;
  PROG = "trap" ;

  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "xt:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'x' : xfersigs = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) dieusage() ;

  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (argc1 + 1 == argc) dieusage() ;
  argv[argc1] = 0 ;
  sigfillset(&full) ;
  sigdelset(&full, SIGKILL) ;
  sigdelset(&full, SIGSTOP) ;
  while (i < argc1)
  {
    unsigned int argc2 ;
    int sig = 0 ;
    if (!sig0_scan(argv[i], &sig) && strcasecmp(argv[i], "default"))
      strerr_dief3x(100, "unrecognized", " directive: ", argv[i]) ;
    argc2 = el_semicolon(argv + ++i) ;
    if (i + argc2 >= argc1)
      strerr_dief3x(100, "unterminated", " internal block for directive ", argv[i-1]) ;
    if (argvs[sig])
      strerr_dief3x(100, "duplicate", " directive: ", argv[i-1]) ;
    if (!sig || (sig != SIGCHLD && sigismember(&full, sig) > 0))
      argvs[sig] = argv + i ;
    else
    {
      char fmt[UINT_FMT] ;
      fmt[uint_fmt(fmt, (unsigned int)sig)] = 0 ;
      strerr_dief5x(100, "SIG", sig_name(sig), " (", fmt, ") cannot be trapped") ;
    }
    argv[i + argc2] = 0 ;
    i += argc2 + 1 ;
  }

  if (argvs[0])
    for (i = 1 ; i < SKALIBS_NSIG ; i++)
      if (!argvs[i] && sigismember(&full, i) > 0)
        argvs[i] = argvs[0] ;

  if (xfersigs) set = full ;
  else
  {
    sigemptyset(&set) ;
    sigaddset(&set, SIGCHLD) ;
    for (i = 1 ; i < SKALIBS_NSIG ; i++)
      if (argvs[i])
        sigaddset(&set, i) ;
  }

  x.fd = selfpipe_init() ;
  if (x.fd < 0) strerr_diefu1sys(111, "selfpipe_init") ;
  if (selfpipe_trapset(&set) < 0) strerr_diefu1sys(111, "trap signals") ;

  pids[SKALIBS_NSIG] = child_spawn0(argv[argc1 + 1], argv + argc1 + 1, envp) ;
  if (!pids[SKALIBS_NSIG]) strerr_diefu2sys(111, "spawn ", argv[argc1 + 1]) ;

 loop:
  if (iopause_g(&x, 1, 0) < 0) strerr_diefu1sys(111, "iopause") ;
  for (;;)
  {
    int r = selfpipe_read() ;
    switch (r)
    {
      case -1 : strerr_diefu1sys(111, "selfpipe_read") ;
      case 0 : goto loop ;
      case SIGCHLD :
        for (;;)
        {
          int wstat ;
          ssize_t id = wait_pids_nohang(pids, SKALIBS_NSIG + 1, &wstat) ;
          if (id < 0 && errno != ECHILD)
            strerr_diefu1sys(111, "wait") ;
          if (id <= 0) break ;
          pids[id - 1] = 0 ;
          if (id == SKALIBS_NSIG + 1) return wait_estatus(wstat) ;
        }
        break ;
      default :
        action(r, envp, envlen) ;
    }
  }
}
