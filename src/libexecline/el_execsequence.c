/* ISC license. */

#include <unistd.h>
#include <skalibs/djbunix.h>
#include <skalibs/env.h>
#include <skalibs/strerr2.h>
#include <skalibs/types.h>
#include <execline/execline.h>

void el_execsequence (char const *const *argv1, char const *const *argv2, char const *const *envp)
{
  size_t j = 2 ;
  int wstat ;
  char fmt[UINT_FMT + 1] = "?=" ;
  pid_t pid = el_spawn0(argv1[0], argv1, envp) ;
  if (!pid) strerr_warnwu2sys("spawn ", argv1[0]) ;
  if (wait_pid(pid, &wstat) < 0)
    strerr_diefu2sys(111, "wait for ", argv1[0]) ;
  if (!argv2[0]) _exit(0) ;
  j += uint_fmt(fmt + j, wait_status(wstat)) ; fmt[j++] = 0 ;
  xpathexec_r(argv2, envp, env_len(envp), fmt, j) ;
}
