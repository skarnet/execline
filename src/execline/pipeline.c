/* ISC license. */

#include <sys/types.h>
#include <unistd.h>
#ifdef EXECLINE_OLD_VARNAMES
#include <skalibs/bytestr.h>
#endif
#include <skalibs/sgetopt.h>
#include <skalibs/uint64.h>
#include <skalibs/strerr2.h>
#include <skalibs/env.h>
#include <skalibs/djbunix.h>
#include <execline/execline.h>

#define USAGE "pipeline [ -d ] [ -r | -w ] { command... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv, char const *const *envp)
{
  int df = 0, w = 0 ;
  PROG = "pipeline" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "drw", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'd' : df = 1 ; break ;
        case 'r' : w = 0 ; break ;
        case 'w' : w = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  {
    pid_t pid ;
    int fd ;
    int argc1 = el_semicolon(argv) ;
    if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
    if (argc1 + 1 == argc) strerr_dief1x(100, "empty remainder") ;
    argv[argc1] = 0 ;
    if (df)
    {
      int p[2] ;
      if (pipe(p) < 0) strerr_diefu1sys(111, "create pipe") ;
      pid = doublefork() ;
      switch (pid)
      {
        case -1: strerr_diefu1sys(111, "doublefork") ;
        case 0:
          PROG = "pipeline (grandchild)" ;
          fd_close(p[w]) ;
          if (fd_move(!w, p[!w]) < 0) strerr_diefu1sys(111, "fd_move") ;
          pathexec0_run(argv, envp) ;
          strerr_dieexec(127, argv[0]) ;
      }
      fd_close(p[!w]) ;
      fd = p[w] ;
    }
    else
    {
      pid = el_spawn1(argv[0], argv, envp, &fd, !w) ;
      if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
    }
    if (fd_move(w, fd) < 0) strerr_diefu1sys(111, "fd_move") ;
    if (w == fd) uncoe(fd) ;
    {
#ifdef EXECLINE_OLD_VARNAMES
      char fmt[UINT64_FMT * 2 + 10] = "!=" ;
#else
      char fmt[UINT64_FMT + 2] = "!=" ;
#endif
      register unsigned int i = 2 ;
      i += uint64_fmt(fmt+i, (uint64)pid) ; fmt[i++] = 0 ;
#ifdef EXECLINE_OLD_VARNAMES
      byte_copy(fmt+i, 8, "LASTPID=") ; i += 8 ;
      i += uint64_fmt(fmt+i, (uint64)pid) ; fmt[i++] = 0 ;
#endif
      pathexec_r(argv + argc1 + 1, envp, env_len(envp), fmt, i) ;
    }
    strerr_dieexec(111, argv[argc1 + 1]) ;
  }
}
