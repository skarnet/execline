/* ISC license. */

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include <skalibs/uint64.h>
#include <skalibs/types.h>
#include <skalibs/envexec.h>
#include <skalibs/tai.h>
#include <skalibs/djbunix.h>
#include <skalibs/selfpipe.h>
#include <skalibs/iopause.h>

#include <execline/execline.h>

#define USAGE "wait [ -u ] [ -a | -o ] [ -t timeout ] { [ pids... ] } [ prog... ]"
#define dieusage() strerr_dieusage(100, USAGE)

typedef pid_t wait_ac_func (void *, int *) ;
typedef wait_ac_func *wait_ac_func_ref ;

enum wait_golb_e
{
  WAIT_GOLB_JUSTONE = 0x01,
  WAIT_GOLB_NOEXPORT = 0x02,
} ;

enum wait_gola_e
{
  WAIT_GOLA_TIMEOUT,
  WAIT_GOLA_N
} ;

struct wait_pidlist_s
{
  pid_t *pids ;
  size_t n ;
  pid_t last ;
  int lastwstat ;
} ;

static inline pid_t wait_hang_nolist_all (int *wstat)
{
  pid_t pid = -1 ;
  int w ;
  for (;;)
  {
    pid_t r = wait_nointr(&w) ;
    if (r == -1) break ;
    pid = r ;
  }
  if (errno != ECHILD) strerr_diefusys(111, "wait") ;
  if (pid > 0) *wstat = w ;
  return pid ;
}

static pid_t wait_nohang_nolist_one (void *data, int *wstat)
{
  pid_t r = wait_nohang(wstat) ;
  if (r == -1 && errno != ECHILD)
    strerr_diefusys(111, "wait_nohang") ;
  (void)data ;
  return r ;
}

static pid_t wait_nohang_nolist_all (void *data, int *wstat)
{
  pid_t *last = data ;
  for (;;)
  {
    pid_t r = wait_nohang(wstat) ;
    if (r == -1) break ;
    else if (!r) return 0 ;
    else *last = r ;
  }
  if (errno != ECHILD) strerr_diefusys(111, "wait_nohang") ;
  return *last ;
}

static pid_t wait_nohang_list_one (void *data, int *wstat)
{
  struct wait_pidlist_s *p = data ;
  while (p->n)
  {
    size_t i ;
    int r = waitpids_nohang(p->pids, p->n, wstat, &i) ;
    if (!r) return 0 ;
    else if (r == -1)
    {
      char fmt[PID_FMT] ;
      fmt[pid_fmt(fmt, p->pids[i])] = 0 ;
      if (errno != ECHILD) strerr_diefusys(111, "wait for pid", fmt) ;
      strerr_warnwusys("wait for pid ", fmt) ;
      p->pids[i] = p->pids[--p->n] ;
    }
    else return p->pids[i] ;
  }
  return -1 ;
}

static pid_t wait_nohang_list_all (void *data, int *wstat)
{
  struct wait_pidlist_s *p = data ;
  while (p->n)
  {
    size_t i ;
    int w ;
    int r = waitpids_nohang(p->pids, p->n, &w, &i) ;
    if (!r) return 0 ;
    else if (r == -1)
    {
      char fmt[PID_FMT] ;
      fmt[pid_fmt(fmt, p->pids[i])] = 0 ;
      if (errno != ECHILD) strerr_diefusys(111, "wait for pid ", fmt) ;
      else strerr_warnwusys("wait for pid ", fmt) ;
    }
    if (p->pids[i] == p->last)
    {
      if (r == -1) p->last = 0 ;
      else p->lastwstat = w ;
    }
    p->pids[i] = p->pids[--p->n] ;
  }
  *wstat = p->lastwstat ;
  return p->last ;
}

static inline void wait_empty_selfpipe (void)
{
  for (;;) switch (selfpipe_read())
  {
    case -1 : strerr_diefu1sys(111, "read selfpipe") ;
    case 0 : return ;
    case SIGCHLD : break ;
    default: strerr_dief(101, "internal inconsistency. Please submit a bug-report.") ;
  }
}

static pid_t wait_with_timeout (wait_ac_func_ref f, void *data, int *wstat, tain *tto)
{
  pid_t pid ;
  iopause_fd x = { .fd = selfpipe_init(), .events = IOPAUSE_READ } ;
  if (x.fd == -1) strerr_diefusys(111, "create selfpipe") ;
  if (!selfpipe_trap(SIGCHLD)) strerr_diefusys(111, "trap SIGCHLD") ;
  tain_now_set_stopwatch_g() ;
  tain_add_g(tto, tto) ;

  for (;;)
  {
    int r ;
    pid = (*f)(data, wstat) ;
    if (pid) break ;
    r = iopause_g(&x, 1, tto) ;
    if (r == -1) strerr_diefusys(111, "iopause") ;
    else if (!r)
    {
      pid = -1 ;
      break ;
    }
    else wait_empty_selfpipe() ;
  }

  selfpipe_finish() ;
  return pid ;
}

int main (int argc, char const **argv)
{
  static gol_bool const rgolb[] =
  {
    { .so = 'a', .lo = "all", .clear = WAIT_GOLB_JUSTONE, .set = 0 },
    { .so = 'o', .lo = "one", .clear = 0, .set = WAIT_GOLB_JUSTONE },
    { .so = 0, .lo = "export", .clear = WAIT_GOLB_NOEXPORT, .set = 0 },
    { .so = 'u', .lo = "no-export", .clear = 0, .set = WAIT_GOLB_NOEXPORT },
  } ;
  static gol_arg const rgola[] =
  {
    { .so = 't', .lo = "timeout", .i = WAIT_GOLA_TIMEOUT },
  } ;
  tain tto = TAIN_INFINITE_RELATIVE ;
  int argc1 ;
  int posixcompat = 0 ;
  pid_t pid ;
  int wstat ;
  uint64_t wgolb = 0 ;
  char const *wgola[WAIT_GOLA_N] = { 0 } ;
  PROG = "wait" ;

  {
    unsigned int golc = GOL_main(argc, argv, rgolb, rgola, &wgolb, wgola) ;
    argc -= golc ; argv += golc ;
  }

  if (wgola[WAIT_GOLA_TIMEOUT])
  {
    unsigned int t = 0 ;
    if (!uint0_scan(wgola[WAIT_GOLA_TIMEOUT], &t)) dieusage() ;
    tain_from_millisecs(&tto, t) ;
  }

  argc1 = el_semicolon_nostrict(argv) ;
  if (argc1 > argc)
  {
    posixcompat = 1 ;
    argc1 = argc ;
  }

  if (argc1)
  {
    pid_t pids[argc1] ;
    struct wait_pidlist_s info = { .pids = pids, .n = argc1 } ;
    for (unsigned int i = 0 ; i < argc1 ; i++)
      if (!pid0_scan(argv[i], pids + i))
        strerr_dief(100, "pids must be numerical") ;
    info.last = pids[argc1 - 1] ;
    pid = wait_with_timeout(wgolb & WAIT_GOLB_JUSTONE ? &wait_nohang_list_one : &wait_nohang_list_all, &info, &wstat, &tto) ;
  }
  else if (wgola[WAIT_GOLA_TIMEOUT])
  {
    pid_t saved = -1 ;
    pid = wait_with_timeout(wgolb & WAIT_GOLB_JUSTONE ? &wait_nohang_nolist_one : &wait_nohang_nolist_all, &saved, &wstat, &tto) ;
  }
  else pid = wgolb & WAIT_GOLB_JUSTONE ? wait_nointr(&wstat) : wait_hang_nolist_all(&wstat) ;

  if (posixcompat) _exit(pid == -1 ? 1 : argc1 ? wait_estatus(wstat) : 0) ;

  if (pid > 0 && !(wgolb & WAIT_GOLB_NOEXPORT))
  {
    char fmt[4 + UINT_FMT + PID_FMT] = "?=" ;
    size_t m = 2 ;
    m += uint_fmt(fmt + m, wait_estatus(wstat)) ; fmt[m++] = 0 ;
    fmt[m++] = '!' ; fmt[m++] = '=' ;
    m += pid_fmt(fmt + m, pid) ; fmt[m++] = 0 ;
    xmexec0_n(argv + argc1 + 1, fmt, m, 2) ;
  }
  else xmexec0_n(argv + argc1 + 1, "?\0!", 4, 2) ;
}
