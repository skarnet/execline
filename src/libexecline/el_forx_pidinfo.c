/* ISC license. */

#include <errno.h>

#include <skalibs/djbunix.h>

#include <execline/execline.h>

el_forx_pidinfo_t *el_forx_pidinfo = 0 ;

int el_forx_isok (unsigned short const *tab, unsigned int n, unsigned short code)
{
  unsigned int i = 0 ;
  for (; i < n ; i++) if (code == tab[i]) break ;
  return i < n ;
}

void el_forx_sigchld_handler (int sig)
{
  int e = errno ;
  for (;;)
  {
    ssize_t r = wait_pids_nohang(el_forx_pidinfo->tab, el_forx_pidinfo->len, &el_forx_pidinfo->wstat) ;
    if (r <= 0) break ;
    el_forx_pidinfo->tab[r-1] = el_forx_pidinfo->tab[--el_forx_pidinfo->len] ;
  }
  errno = e ;
  (void)sig ;
}
