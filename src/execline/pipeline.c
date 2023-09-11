/* ISC license. */

#include <unistd.h>
#include <errno.h>

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr.h>
#include <skalibs/cspawn.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

#define USAGE "pipeline [ -d ] [ -r | -w ] { command... } command..."
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const **argv, char const *const *envp)
{
  cspawn_fileaction fa[2] =
  {
    [0] = { .type = CSPAWN_FA_CLOSE },
    [1] = { .type = CSPAWN_FA_MOVE }
  } ;
  pid_t pid ;
  int p[2] ;
  int argc1 ;
  int df = 0, w = 0 ;
  size_t i = 2 ;
  char fmt[PID_FMT + 2] = "!=" ;

  PROG = "pipeline" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "drw", &l) ;
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

  argc1 = el_semicolon(argv) ;
  if (!argc1) strerr_dief1x(100, "empty block") ;
  if (argc1 >= argc) strerr_dief1x(100, "unterminated block") ;
  if (argc1 + 1 == argc) strerr_dief1x(100, "empty remainder") ;
  argv[argc1] = 0 ;
  if (pipe(p) == -1) strerr_diefu1sys(111, "create pipe") ;
  fa[0].x.fd = p[w] ;
  fa[1].x.fd2[0] = !w ;
  fa[1].x.fd2[1] = p[!w] ;
  pid = df ? gcspawn(argv[0], argv, envp, 0, fa, 2) : cspawn(argv[0], argv, envp, 0, fa, 2) ;
  if (!pid) strerr_diefu2sys(111, "spawn ", argv[0]) ;
  fd_close(p[!w]) ;
  if (fd_move(w, p[w]) == -1) strerr_diefu1sys(111, "fd_move") ;
  if (w == p[w]) uncoe(w) ;
  i += pid_fmt(fmt+i, pid) ; fmt[i++] = 0 ;
  xmexec_en(argv + argc1 + 1, envp, fmt, i, 1) ;
}
