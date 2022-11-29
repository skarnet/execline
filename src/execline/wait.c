/* ISC license. */

#include <execline/config.h>

#include <sys/wait.h>
#include <errno.h>
#ifdef EXECLINE_PEDANTIC_POSIX
#include <locale.h>
#endif

#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/tai.h>
#include <skalibs/djbunix.h>
#include <skalibs/selfpipe.h>
#include <skalibs/iopause.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "wait [ -I | -i ] [ -a | -o ] [ -r | -t timeout ] { pids... } [ prog... ]"
#define dieusage() strerr_dieusage(100, USAGE)

typedef int ac_func (pid_t *, unsigned int *, int *) ;
typedef ac_func *ac_func_ref ;

static int wait_all (void)
{
  errno = 0 ;
  while (wait_nointr(0) > 0) ;
  if (errno != ECHILD) strerr_diefu1sys(111, "wait") ;
  return 0 ;
}

static int wait_from_list (pid_t *pids, unsigned int n)
{
  int wstat = -1 ;
  if (!waitn_posix(pids, n, &wstat) && errno != ECHILD)
    strerr_diefu1sys(111, "wait") ;
  return wstat == -1 ? -1 : wait_estatus(wstat) ;
}

static int wait_one (pid_t *pid)
{
  int wstat = 0 ;
  *pid = wait(&wstat) ;
  if (*pid < 0)
  {
    if (errno != ECHILD) strerr_diefu1sys(111, "wait") ;
    else return -1 ;
  }
  return wait_estatus(wstat) ;
}

static int wait_one_from_list (pid_t *pids, unsigned int n, pid_t *pid)
{
  for (;;)
  {
    int wstat ;
    pid_t r = wait(&wstat) ;
    unsigned int i = 0 ;
    if (r < 0)
    {
      if (errno != ECHILD) strerr_diefu1sys(111, "wait") ;
      else return -1 ;
    }
    for (; i < n ; i++) if (r == pids[i]) break ;
    if (i < n)
    {
      *pid = r ;
      pids[i] = pids[n-1] ;
      return wait_estatus(wstat) ;
    }
  }
}

static int wait_one_nohang (pid_t *pids, unsigned int *n, int *wstat)
{
  pid_t r = wait_nohang(wstat) ;
  if (r < 0 && errno != ECHILD) strerr_diefu1sys(111, "wait") ;
  (void)pids ;
  (void)n ;
  return r ;
}

static int wait_one_from_list_nohang (pid_t *pids, unsigned int *n, int *wstat)
{
  for (;;)
  {
    pid_t r = wait_nohang(wstat) ;
    unsigned int i = 0 ;
    if (r <= 0)
    {
      if (r == -1 && errno != ECHILD) strerr_diefu1sys(111, "wait") ;
      else return r ;
    }
    for (; i < *n ; i++) if (r == pids[i]) break ;
    if (i < *n)
    {
      pids[i] = pids[--*n] ;
      return r ;
    }
  }
}

static inline void empty_selfpipe (void)
{
  for (;;) switch (selfpipe_read())
  {
    case -1 : strerr_diefu1sys(111, "read selfpipe") ;
    case 0 : return ;
    case SIGCHLD : break ;
    default: strerr_dief1x(101, "internal inconsistency. Please submit a bug-report.") ;
  }
}

static int wait_with_timeout (pid_t *pids, unsigned int n, pid_t *returned, ac_func_ref f, tain *tto, int justone, int strict)
{
  iopause_fd x = { .fd = selfpipe_init(), .events = IOPAUSE_READ } ;
  pid_t special = pids[n-1] ;
  int e = special ? -1 : 0 ;
  if (x.fd < 0) strerr_diefu1sys(111, "create selfpipe") ;
  if (!selfpipe_trap(SIGCHLD)) strerr_diefu1sys(111, "trap SIGCHLD") ;
  tain_now_set_stopwatch_g() ;
  tain_add_g(tto, tto) ;
  for (;;)
  {
    int r ;
    for (;;)
    {
      int wstat ;
      pid_t pid = (*f)(pids, &n, &wstat) ;
      if (!pid) break ;
      if (pid < 0) goto endloop ;
      if (justone || pid == special)
      {
        *returned = pid ;
        e = wait_estatus(wstat) ;
      }
      if (justone || !n) goto endloop ;
    }
    r = iopause_g(&x, 1, tto) ;
    if (r < 0) strerr_diefu1sys(111, "iopause") ;
    else if (!r)
    {
      if (!strict) { e = -1 ; break ; }
      errno = ETIMEDOUT ;
      strerr_diefu1sys(99, "wait") ;
    }
    else empty_selfpipe() ;
  }
 endloop:
  selfpipe_finish() ;
  return e ;
}

int main (int argc, char const **argv)
{
  tain tto ;
  int argc1 ;
  int hastimeout = 0 ;
  int insist = 0 ;
  int justone = 0 ;
  int hasblock ;
  int e ;
  pid_t pid = -1 ;
  PROG = "wait" ;
#ifdef EXECLINE_PEDANTIC_POSIX
  setlocale(LC_ALL, "") ;  /* but of course, dear POSIX */
#endif
  {
    subgetopt l = SUBGETOPT_ZERO ;
    unsigned int t = 0 ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "iIaort:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'i' : insist = 1 ; break ;
        case 'I' : insist = 0 ; break ;
        case 'a' : justone = 0 ; break ;
        case 'o' : justone = 1 ; break ;
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
  if (argc1 >= argc)
  {
    hasblock = 0 ;
    argc1 = argc ;
  }
  else hasblock = 1 ;

  {
    unsigned int n = argc1 ? argc1 : 1 ;
    pid_t pids[n] ;
    if (argc1)  
    {
      unsigned int i = 0 ;
      for (; i < n ; i++)
        if (!pid0_scan(argv[i], pids + i)) strerr_dieusage(100, USAGE) ;
    }
    else pids[0] = 0 ;

    e = hastimeout ?  /* wait -t30000 whatever */
         wait_with_timeout(pids, n, &pid, argc1 ? &wait_one_from_list_nohang : &wait_one_nohang, &tto, justone, insist) :
         justone ?
           argc1 ?
             wait_one_from_list(pids, n, &pid) :  /* wait -o -- 2 3 4 / wait -o -- { 2 3 4 } */
             wait_one(&pid) :  /* wait -o / wait -o { } */
           argc1 ?
             wait_from_list(pids, n) :  /* wait 2 3 4 / wait { 2 3 4 }*/
             wait_all() ; /* wait / wait { } */
  }
  if (!hasblock) return e >= 0 ? e : 127 ;
  if (!justone) xexec0(argv + argc1 + 1) ;
  if (e < 0) xmexec0_n(argv + argc1 + 1, "?\0!", 4, 2) ;

  {
    char fmt[4 + UINT_FMT + PID_FMT] = "?=" ;
    size_t m = 2 ;
    m += uint_fmt(fmt + m, (unsigned int)e) ; fmt[m++] = 0 ;
    fmt[m++] = '!' ; fmt[m++] = '=' ;
    m += pid_fmt(fmt + m, pid) ; fmt[m++] = 0 ;
    xmexec0_n(argv + argc1 + 1, fmt, m, 2) ;
  }
}
