/* ISC license. */

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <skalibs/types.h>
#include <skalibs/strerr.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#include <execline/execline.h>

void el_execsequence (char const *const *argv1, char const *const *argv2, char const *const *envp)
{
  size_t j = 2 ;
  char fmt[UINT_FMT + 2] = "?=" ;
  pid_t pid = el_spawn0(argv1[0], argv1, envp) ;
  if (!pid)
  {
    fmt[j++] = '1' ;
    fmt[j++] = '2' ;
    fmt[j++] = '6' + (errno == ENOENT) ;
    strerr_warnwu2sys("spawn ", argv1[0]) ;
  }
  else
  {
    int wstat ;
    if (wait_pid(pid, &wstat) < 0)
      strerr_diefu2sys(111, "wait for ", argv1[0]) ;
    j += uint_fmt(fmt + j, wait_status(wstat)) ;
  }
  fmt[j++] = 0 ;
  xmexec0_en(argv2, envp, fmt, j, 1) ;
}
