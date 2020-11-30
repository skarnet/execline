/* ISC license. */

#include <fcntl.h>
#include <errno.h>

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "redirfd -[ r | w | u | a | x ] [ -n ] [ -b ] fd file prog..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  int fd, fd2 ;
  unsigned int flags = 0 ;
  int what = -1 ;
  int changemode = 0 ;
  PROG = "redirfd" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "rwuaxnb", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'r' : what = O_RDONLY ; flags &= ~(O_APPEND|O_CREAT|O_TRUNC|O_EXCL) ; break ;
        case 'w' : what = O_WRONLY ; flags |= O_CREAT|O_TRUNC ; flags &= ~(O_APPEND|O_EXCL) ; break ;
        case 'u' : what = O_RDWR ; flags &= ~(O_APPEND|O_CREAT|O_TRUNC|O_EXCL) ; break ;
        case 'a' : what = O_WRONLY ; flags |= O_CREAT|O_APPEND ; flags &= ~(O_TRUNC|O_EXCL) ; break ;
        case 'x' : what = O_WRONLY ; flags |= O_CREAT|O_EXCL ; flags &= ~(O_APPEND|O_TRUNC) ; break ;
        case 'n' : flags |= O_NONBLOCK ; break ;
        case 'b' : changemode = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if ((argc < 3) || (what == -1)) dieusage() ;
  if (!uint0_scan(argv[0], (unsigned int *)&fd)) dieusage() ;
  flags |= what ;
  fd2 = open3(argv[1], flags, 0666) ;
  if ((fd2 == -1) && (what == O_WRONLY) && (errno == ENXIO))
  {
    int fdr = open_read(argv[1]) ;
    if (fdr == -1) strerr_diefu2sys(111, "open_read ", argv[1]) ;
    fd2 = open3(argv[1], flags, 0666) ;
    fd_close(fdr) ;
  }
  if (fd2 == -1) strerr_diefu2sys(111, "open ", argv[1]) ;
  if (fd_move(fd, fd2) == -1)
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, fd2)] = 0 ;
    strerr_diefu4sys(111, "move fd ", fmt, " to fd ", argv[0]) ;
  }
  if (changemode)
  {
    if (((flags & O_NONBLOCK) ? ndelay_off(fd) : ndelay_on(fd)) < 0)
      strerr_diefu1sys(111, "change blocking mode") ;
  }
  xexec(argv+2) ;
}
