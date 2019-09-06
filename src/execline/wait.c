/* ISC license. */

#include <sys/wait.h>
#include <errno.h>
#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/tai.h>
#include <skalibs/djbunix.h>
#include <skalibs/selfpipe.h>
#include <skalibs/iopause.h>
#include <execline/execline.h>

#define USAGE "wait [ -I | -i ] [ -r | -t timeout ] { pids... }"
#define dieusage() strerr_dieusage(100, USAGE)

typedef int actfunc_t (pid_t *, unsigned int *) ;
typedef actfunc_t *actfunc_t_ref ;

static inline void waitall (void)
{
  pid_t r = 1 ;
  while (r > 0) r = wait(0) ;
  if (r < 0 && errno != ECHILD) strerr_diefu1sys(111, "wait") ;
}

static int waitany (pid_t *dummytab, unsigned int *dummyn)
{
  pid_t r = 1 ;
  while (r > 0) r = wait_nohang(0) ;
  if (!r) return 1 ;
  if (errno != ECHILD) strerr_diefu1sys(111, "wait") ;
  (void)dummytab ;
  (void)dummyn ;
  return 0 ;
}

static int waitintab (pid_t *tab, unsigned int *n)
{
  unsigned int i = 0 ;
  for (; i < *n ; i++)
  {
    pid_t r = waitpid(tab[i], 0, WNOHANG) ;
    if (r)
    {
      if (r < 0 && errno != ECHILD) strerr_diefu1sys(111, "waitpid") ;
      tab[i--] = tab[--(*n)] ;
    }
  }
  return !!*n ;
}

static inline void handle_signals (void)
{
  for (;;) switch (selfpipe_read())
  {
    case -1 : strerr_diefu1sys(111, "read selfpipe") ;
    case 0 : return ;
    case SIGCHLD : break ;
    default: strerr_dief1x(101, "internal inconsistency. Please submit a bug-report.") ;
  }
}

static inline void mainloop (tain_t *deadline, int insist, actfunc_t_ref f, pid_t *tab, unsigned int *n)
{
  iopause_fd x = { .events = IOPAUSE_READ } ;
  x.fd = selfpipe_init() ;
  if (x.fd < 0) strerr_diefu1sys(111, "create selfpipe") ;
  if (selfpipe_trap(SIGCHLD) < 0) strerr_diefu1sys(111, "trap SIGCHLD") ;
  tain_now_set_stopwatch_g() ;
  tain_add_g(deadline, deadline) ;
  while ((*f)(tab, n))
  {
    int r = iopause_g(&x, 1, deadline) ;
    if (r < 0) strerr_diefu1sys(111, "iopause") ;
    else if (!r)
    {
      if (!insist) break ;
      errno = ETIMEDOUT ;
      strerr_diefu1sys(1, "wait") ;
    }
    else handle_signals() ;
  }
  selfpipe_finish() ;
}

int main (int argc, char const **argv, char const *const *envp)
{
  tain_t tto ;
  int argc1 ;
  int hastimeout = 0 ;
  int insist = 0 ;
  PROG = "wait" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    unsigned int t = 0 ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "iIrt:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'i' : insist = 1 ; break ;
        case 'I' : insist = 0 ; break ;
        case 'r' : t = 0 ; hastimeout = 1 ; break ;
        case 't' : if (!uint0_scan(l.arg, &t)) dieusage() ; hastimeout = 1 ; break ;    
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
    if (hastimeout) tain_from_millisecs(&tto, t) ;
    else tto = tain_infinite_relative ;
  }
  argc1 = el_semicolon(argv) ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (!argc1 && !hastimeout) waitall() ;
  else
  {
    actfunc_t_ref f = argc1 ? &waitintab : &waitany ;
    unsigned int n = argc1 ? (unsigned int)argc1 : 1 ;
    pid_t tab[n] ;
    if (argc1)
    {
      unsigned int i = 0 ;
      for (; i < n ; i++)
        if (!pid0_scan(argv[i], tab+i)) strerr_dieusage(100, USAGE) ;
    }
    mainloop(&tto, insist, f, tab, &n) ;
  }

  xpathexec0_run(argv + argc1 + 1, envp) ;
}
