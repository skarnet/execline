/* ISC license. */

#include <sys/types.h>
#ifdef EXECLINE_OLD_VARNAMES
#include <skalibs/bytestr.h>
#endif
#include <skalibs/djbunix.h>
#include <skalibs/env.h>
#include <skalibs/strerr2.h>
#include <skalibs/uint.h>
#include <execline/execline.h>

void el_execsequence (char const *const *argv1, char const *const *argv2, char const *const *envp)
{
  if (!argv2[0])
  {
    pathexec0_run(argv1, envp) ;
    strerr_dieexec(111, argv1[0]) ;
  }
  else
  {
    int wstat ;
    unsigned int j = 2 ;
#ifdef EXECLINE_OLD_VARNAMES
    char fmt[UINT_FMT * 2 + 15] = "?=" ;
#else
    char fmt[UINT_FMT + 1] = "?=" ;
#endif
    pid_t pid = el_spawn0(argv1[0], argv1, envp) ;
    if (!pid) strerr_warnwu2sys("spawn ", argv1[0]) ;
    if (wait_pid(pid, &wstat) < 0)
      strerr_diefu2sys(111, "wait for ", argv1[0]) ;
    j += uint_fmt(fmt + j, wait_status(wstat)) ; fmt[j++] = 0 ;
#ifdef EXECLINE_OLD_VARNAMES
    byte_copy(fmt + j, 13, "LASTEXITCODE=") ; j += 13 ;
    j += uint_fmt(fmt + j, wait_status(wstat)) ; fmt[j++] = 0 ;
#endif
    pathexec_r(argv2, envp, env_len(envp), fmt, j) ;
  }
  strerr_dieexec(111, argv2[0]) ;
}
