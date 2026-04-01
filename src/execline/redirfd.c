/* ISC license. */

#include <fcntl.h>
#include <errno.h>

#include <skalibs/uint64.h>
#include <skalibs/types.h>
#include <skalibs/djbunix.h>
#include <skalibs/envexec.h>

#include <execline/execline.h>

#define USAGE "redirfd -[ r | w | u | a | x ] [ -N | -n ] [ -b ] [ -v ] [ -E | -e ] fd file prog..."
#define dieusage() strerr_dieusage(100, USAGE)

enum golb_e
{
  GOLB_READ = 0x0001,
  GOLB_WRITE = 0x0002,
  GOLB_CREAT = 0x0004,
  GOLB_APPEND = 0x0008,
  GOLB_TRUNC = 0x0010,
  GOLB_EXCL = 0x0020,
  GOLB_NONBLOCK = 0x0040,
  GOLB_CHANGEMODE = 0x0100,
  GOLB_VAR = 0x0200,
  GOLB_AUTOIMPORT = 0x0400,
} ;

int main (int argc, char const *const *argv)
{
  static gol_bool const rgolb[] =
  {
    { .so = 'r', .lo = 0, .clear = GOLB_WRITE | GOLB_APPEND | GOLB_CREAT | GOLB_TRUNC | GOLB_EXCL, .set = GOLB_READ },
    { .so = 'w', .lo = 0, .clear = GOLB_READ | GOLB_APPEND | GOLB_EXCL, .set = GOLB_WRITE | GOLB_CREAT | GOLB_TRUNC },
    { .so = 'u', .lo = 0, .clear = GOLB_APPEND | GOLB_CREAT | GOLB_TRUNC | GOLB_EXCL, .set = GOLB_READ | GOLB_WRITE },
    { .so = 'a', .lo = 0, .clear = GOLB_READ | GOLB_TRUNC | GOLB_EXCL, .set = GOLB_WRITE | GOLB_CREAT | GOLB_APPEND },
    { .so = 'x', .lo = 0, .clear = GOLB_READ | GOLB_APPEND | GOLB_TRUNC, .set = GOLB_WRITE | GOLB_CREAT | GOLB_EXCL },
    { .so = 'N', .lo = "block", .clear = GOLB_NONBLOCK, .set = 0 },
    { .so = 'n', .lo = "nonblock", .clear = 0, .set = GOLB_NONBLOCK },
    { .so = 'b', .lo = "switch-block", .clear = 0, .set = GOLB_CHANGEMODE },
    { .so = 'v', .lo = "variable", .clear = 0, .set = GOLB_VAR },
    { .so = 'e', .lo = "no-autoimport", .clear = GOLB_AUTOIMPORT, .set = 0 },
    { .so = 'E', .lo = "autoimport", .clear = 0, .set = GOLB_AUTOIMPORT },
    { .so = 0, .lo = "no-read", .clear = GOLB_READ, .set = 0 },
    { .so = 0, .lo = "read", .clear = 0, .set = GOLB_READ },
    { .so = 0, .lo = "no-write", .clear = GOLB_WRITE, .set = 0 },
    { .so = 0, .lo = "write", .clear = 0, .set = GOLB_WRITE },
    { .so = 0, .lo = "no-create", .clear = GOLB_CREAT, .set = 0 },
    { .so = 0, .lo = "create", .clear = 0, .set = GOLB_CREAT },
    { .so = 0, .lo = "no-append", .clear = GOLB_APPEND, .set = 0 },
    { .so = 0, .lo = "append", .clear = 0, .set = GOLB_APPEND },
    { .so = 0, .lo = "no-trunc", .clear = GOLB_TRUNC, .set = 0 },
    { .so = 0, .lo = "trunc", .clear = 0, .set = GOLB_TRUNC },
    { .so = 0, .lo = "no-excl", .clear = GOLB_EXCL, .set = 0 },
    { .so = 0, .lo = "excl", .clear = 0, .set = GOLB_EXCL },
  } ;
  uint64_t wgolb = 0 ;
  unsigned int golc ;
  int fd ;
  unsigned int fdto ;
  unsigned int flags ;
  PROG = "redirfd" ;

  golc = gol_main(argc, argv, rgolb, sizeof(rgolb)/sizeof(gol_bool), 0, 0, &wgolb, 0) ;
  argc -= golc ; argv += golc ;
  if (argc < 3) dieusage() ;
  if (!(wgolb & (GOLB_READ | GOLB_WRITE))) dieusage() ;
  if (!(wgolb & GOLB_VAR))
    if (!uint0_scan(argv[0], &fdto)) dieusage() ;

  flags =
    (wgolb & GOLB_WRITE ? wgolb & GOLB_READ ? O_RDWR : O_WRONLY : O_RDONLY) |
    (wgolb & GOLB_CREAT ? O_CREAT : 0) |
    (wgolb & GOLB_APPEND ? O_APPEND : 0) |
    (wgolb & GOLB_TRUNC ? O_TRUNC : 0) |
    (wgolb & GOLB_EXCL ? O_EXCL : 0) |
    (wgolb & GOLB_NONBLOCK ? O_NONBLOCK : 0) ;

  fd = open3(argv[1], flags, 0666) ;
  if (fd == -1 && wgolb & GOLB_WRITE && errno == ENXIO)
  {
    int fdr = open_read(argv[1]) ;
    if (fdr == -1) strerr_diefu2sys(111, "open_read ", argv[1]) ;
    fd = open3(argv[1], flags, 0666) ;
    fd_close(fdr) ;
  }
  if (fd == -1) strerr_diefu2sys(111, "open ", argv[1]) ;

  if (wgolb & GOLB_CHANGEMODE)
    if ((wgolb & GOLB_NONBLOCK ? ndelay_off(fd) : ndelay_on(fd)) == -1)
      strerr_diefu1sys(111, "change blocking mode") ;
  if (wgolb & GOLB_VAR)
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, fd)] = 0 ;
    el_modif_and_exec(argv+2, argv[0], fmt, !!(wgolb & GOLB_AUTOIMPORT)) ;
  }
  else
  {
    if (fd_move(fdto, fd) == -1) strerr_diefu1sys(111, "fd_move") ;
    xexec(argv+2) ;
  }
}
