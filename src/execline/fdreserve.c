/* ISC license. */

#include <unistd.h>
#include <sys/resource.h>

#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "fdreserve n prog..."

#define MAXFDS 1024

unsigned int doit (char *modif, unsigned int i, int fd)
{
  unsigned int pos = 2 ;
  modif[0] = 'F' ; modif[1] = 'D' ;
  pos += uint_fmt(modif + pos, i) ;
  modif[pos++] = '=' ;
  pos += uint_fmt(modif + pos, (unsigned int)fd) ;
  modif[pos++] = 0 ;
  return pos ;
}

int main (int argc, char const *const *argv)
{
  unsigned int n ;
  PROG = "fdreserve" ;
  if ((argc < 3) || !uint0_scan(argv[1], &n))
    strerr_dieusage(100, USAGE) ;
  {
    struct rlimit lim ;
    if (getrlimit(RLIMIT_NOFILE, &lim) < 0) strerr_diefu1sys(111, "getrlimit") ;
    if (n > lim.rlim_cur) strerr_dief1x(100, "too many requested fds") ;
  }
  {
    char modif[12 * n] ;  /* enough for n times "FDaaaa=bbbb\0" */
    unsigned int j = 0 ;
    {
      int fd[n >> 1][2] ;
      unsigned int i = 0 ;
      for (; i < (n>>1) ; i++)
        if (pipe(fd[i]) < 0)
          strerr_diefu1sys(111, "reserve fds") ;
      if (n & 1)
      {
        int lastfd = openb_read("/dev/null") ;
        if (lastfd < 0)
          strerr_diefu1sys(111, "reserve last fd") ;
        fd_close(lastfd) ;
        j += doit(modif + j, n-1, lastfd) ;
      }
      for (i = 0 ; i < (n>>1) ; i++)
      {
        fd_close(fd[i][0]) ;
        fd_close(fd[i][1]) ;
        j += doit(modif + j, i<<1, fd[i][0]) ;
        j += doit(modif + j, (i<<1)|1, fd[i][1]) ;
      }
    }
    xmexec_n(argv+2, modif, j, n) ;
  }
}
