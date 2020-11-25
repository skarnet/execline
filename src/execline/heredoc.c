/* ISC license. */

#include <string.h>
#include <unistd.h>

#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/types.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "heredoc [ -d ] fd string command..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  int df = 0 ;
  PROG = "heredoc" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "d", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'd' : df = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 3) dieusage() ;
  {
    int fd[2] ;
    unsigned int fdr ;
    pid_t pid ;
    if (!uint0_scan(argv[0], &fdr)) strerr_dieusage(100, USAGE) ;
    if (pipe(fd) < 0) strerr_diefu1sys(111, "pipe") ;
    pid = df ? doublefork() : fork() ;
    switch (pid)
    {
      case -1: strerr_diefu2sys(111, df ? "double" : "", "fork") ;
      case 0:
      {
        size_t len = strlen(argv[1]) ;
        PROG = "heredoc (child)" ;
        fd_close(fd[0]) ;
        if (allwrite(fd[1], argv[1], len) < len)
          strerr_diefu1sys(111, "allwrite") ;
        return 0 ;
      }
    }
    fd_close(fd[1]) ;
    if (fd_move(fdr, fd[0]) == -1)
      strerr_diefu2sys(111, "read on fd ", argv[0]) ;
  }
  xexec(argv+2) ;
}
