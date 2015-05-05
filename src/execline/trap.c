/* ISC license. */

#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/uint.h>
#include <skalibs/nsig.h>
#include <skalibs/sig.h>
#include <skalibs/strerr2.h>
#include <skalibs/tai.h>
#include <skalibs/iopause.h>
#include <skalibs/selfpipe.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

#define USAGE "trap [ -x ] [ -t timeout ] { signal { cmdline } ... } prog..."
#define dieusage() strerr_dieusage(100, USAGE) ;

static pid_t pids[NSIG + 2] ;
static char const *const *argvs[NSIG + 1] ;

static void action (unsigned int i, char const *const *envp)
{
  if (argvs[i])
  {
    if (!pids[i])
    {
      pids[i] = child_spawn0(argvs[i][0], argvs[i], envp) ;
      if (!pids[i]) strerr_diefu2sys(111, "spawn ", argvs[i][0]) ;
    }
  }
  else kill(pids[NSIG+1], i) ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  tain_t tto ;
  int xfersigs = 0 ;
  int argc1, spfd ;
  unsigned int i = NSIG + 2 ;
  PROG = "trap" ;
  {
    unsigned int t = 0 ;
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "xt:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'x' : xfersigs = 1 ; break ;
        case 't' : if (!uint0_scan(l.arg, &t)) dieusage() ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
    if (t) tain_from_millisecs(&tto, t) ;
    else tto = tain_infinite_relative ;
  }

  if (!argc) dieusage() ;
  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (argc1 + 1 == argc) dieusage() ;
  argv[argc1] = 0 ;
  while (i--) argvs[i] = 0 ;
  i = 0 ;
  tain_now_g() ;
  spfd = selfpipe_init() ;
  if (spfd < 0) strerr_diefu1sys(111, "selfpipe_init") ;

  while (i < (unsigned int)argc1)
  {
    int argc2 ;
    unsigned int sig = (unsigned int)sig_number(argv[i] + (case_diffb(argv[i], 3, "sig") ? 0 : 3)) ;
    if (!sig && !uint0_scan(argv[i], &sig) && case_diffs(argv[i], "timeout"))
      strerr_dief3x(100, "unrecognized", " directive: ", argv[i]) ;
    argc2 = el_semicolon(argv + ++i) ;
    if (!argc2)
      strerr_dief3x(100, "empty", " internal block for directive ", argv[i-1]) ;
    if (i + argc2 >= argc1)
      strerr_dief3x(100, "unterminated", " internal block for directive ", argv[i-1]) ;
    if (argvs[sig])
      strerr_dief3x(100, "duplicate", " directive: ", argv[i-1]) ;
    if (sig && selfpipe_trap(sig) < 0)
      strerr_diefu2sys(111, "trap ", argv[i-1]) ;
    argv[i + argc2] = 0 ;
    argvs[sig] = argv + i ;
    i += argc2 + 1 ;
  }
  if (!argvs[SIGCHLD] && selfpipe_trap(SIGCHLD) < 0)
    strerr_diefu2sys(111, "trap ", "SIGCHLD") ;

  if (xfersigs)
  {
    sigset_t full ;
    sigfillset(&full) ;
    sigdelset(&full, SIGCHLD) ;
    for (i = 1 ; i <= NSIG ; i++)
      if (!argvs[i] && sigismember(&full, i) > 0 && selfpipe_trap(i) < 0)
      {
        char fmt[UINT_FMT] ;
        fmt[uint_fmt(fmt, i)] = 0 ;
        strerr_diefu4sys(111, "auto-", "trap ", "signal ", fmt) ;
      }
  }

  pids[NSIG+1] = child_spawn0(argv[argc1 + 1], argv + argc1 + 1, envp) ;
  if (!pids[NSIG+1]) strerr_diefu2sys(111, "spawn ", argv[argc1 + 1]) ;

  {
    iopause_fd x = { .fd = spfd, .events = IOPAUSE_READ } ;
    char modif[2 + UINT64_FMT] = "!=" ;
    unsigned int envlen = env_len(envp) ;
    char const *newenvp[envlen + 2] ;
    i = 2 + uint64_fmt(modif + 2, pids[NSIG+1]) ;
    modif[i++] = 0 ;
    if (!env_merge(newenvp, envlen + 2, envp, envlen, modif, i))
      strerr_diefu1sys(111, "adjust environment") ;
    for (;;)
    {
      tain_t deadline ;
      register int r ;
      tain_add_g(&deadline, &tto) ;
      r = iopause_g(&x, 1, &deadline) ;
      if (r < 0) strerr_diefu1sys(111, "iopause") ;
      if (!r) action(0, newenvp) ;
      else
      {
        int cont = 1 ;
        while (cont)
        {
          r = selfpipe_read() ;
          switch (r)
          {
            case -1 : strerr_diefu1sys(111, "selfpipe_read") ;
            case 0 : cont = 0 ; break ;
            case SIGCHLD :
              for (;;)
              {
                int id, wstat ;
                id = wait_pids_nohang(pids, NSIG + 2, &wstat) ;
                if (id < 0 && errno != ECHILD)
                  strerr_diefu1sys(111, "wait") ;
                if (id <= 0) break ;
                pids[id - 1] = 0 ;
                if (id == NSIG + 2) return wait_estatus(wstat) ;
              }
              if (!argvs[SIGCHLD]) break ;
            default :
              action(r, newenvp) ;
          }
        }
      }
    }
  }
}
