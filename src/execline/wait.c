/* ISC license. */

#include <unistd.h>

#include <sys/wait.h>
#include <errno.h>
#ifdef EXECLINE_PEDANTIC_POSIX
#include <locale.h>
#endif

#include <skalibs/types.h>
#include <skalibs/envexec.h>
#include <skalibs/tai.h>
#include <skalibs/djbunix.h>
#include <skalibs/selfpipe.h>
#include <skalibs/iopause.h>

#include <execline/config.h>
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
  if (*pid == -1)
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
    if (r == -1)
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
  if (r == -1 && errno != ECHILD) strerr_diefu1sys(111, "wait") ;
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
  if (x.fd == -1) strerr_diefu1sys(111, "create selfpipe") ;
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
    if (r == -1) strerr_diefu1sys(111, "iopause") ;
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

enum wait_golb_e
{
  GOLB_INSIST = 0x01,
  GOLB_JUSTONE = 0x02,
  GOLB_REAP = 0x04,
} ;

enum wait_gola_e
{
  GOLA_TIMEOUT,
  GOLA_N
} ;

int main (int argc, char const **argv)
{
  static gol_bool const rgolb[] =
  {
    { .so = 'I', .lo = "no-insist", .clear = GOLB_INSIST, .set = 0 },
    { .so = 'i', .lo = "insist", .clear = 0, .set = GOLB_INSIST },
    { .so = 'a', .lo = "all", .clear = GOLB_JUSTONE, .set = 0 },
    { .so = 'o', .lo = "one", .clear = 0, .set = GOLB_JUSTONE },
    { .so = 'r', .lo = "reap", .clear = 0, .set = GOLB_REAP },
  } ;
  static gol_arg const rgola[] =
  {
    { .so = 't', .lo = "timeout", .i = GOLA_TIMEOUT },
  } ;
  tain tto = TAIN_INFINITE_RELATIVE ;
  int argc1 ;
  int hasblock = 1 ;
  int e ;
  uint64_t wgolb = 0 ;
  char const *wgola[GOLA_N] = { 0 } ;
  unsigned int golc ;
  pid_t pid = -1 ;
  PROG = "wait" ;
#ifdef EXECLINE_PEDANTIC_POSIX
  setlocale(LC_ALL, "") ;  /* but of course, dear POSIX */
#endif
  golc = GOL_main(argc, argv, rgolb, rgola, &wgolb, wgola) ;
  argc -= golc ; argv += golc ;

  if (wgolb & GOLB_REAP) wgola[GOLA_TIMEOUT] = "0" ;
  if (wgola[GOLA_TIMEOUT])
  {
    unsigned int t = 0 ;
    if (!uint0_scan(wgola[GOLA_TIMEOUT], &t)) dieusage() ;
    tain_from_millisecs(&tto, t) ;
  }

  argc1 = el_semicolon_nostrict(argv) ;
  if (argc1 > argc)
  {
    hasblock = 0 ;
    argc1 = argc ;
  }

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

    e = wgola[GOLA_TIMEOUT] ?  /* wait -t30000 whatever */
         wait_with_timeout(pids, n, &pid, argc1 ? &wait_one_from_list_nohang : &wait_one_nohang, &tto, !!(wgolb & GOLB_JUSTONE), !!(wgolb & GOLB_INSIST)) :
         wgolb & GOLB_JUSTONE ?
           argc1 ?
             wait_one_from_list(pids, n, &pid) :  /* wait -o -- 2 3 4 / wait -o -- { 2 3 4 } */
             wait_one(&pid) :  /* wait -o / wait -o { } */
           argc1 ?
             wait_from_list(pids, n) :  /* wait 2 3 4 / wait { 2 3 4 }*/
             wait_all() ; /* wait / wait { } */
  }
  if (!hasblock) _exit(e >= 0 ? e : 127) ;
  if (!(wgolb & GOLB_JUSTONE)) xexec0(argv + argc1 + 1) ;
  if (e == -1) xmexec0_n(argv + argc1 + 1, "?\0!", 4, 2) ;

  {
    char fmt[4 + UINT_FMT + PID_FMT] = "?=" ;
    size_t m = 2 ;
    m += uint_fmt(fmt + m, (unsigned int)e) ; fmt[m++] = 0 ;
    fmt[m++] = '!' ; fmt[m++] = '=' ;
    m += pid_fmt(fmt + m, pid) ; fmt[m++] = 0 ;
    xmexec0_n(argv + argc1 + 1, fmt, m, 2) ;
  }
}
