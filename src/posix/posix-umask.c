/* ISC license. */

#include <stdint.h>
#include <sys/stat.h>
#include <locale.h>

#include <skalibs/gccattributes.h>
#include <skalibs/bytestr.h>
#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#define USAGE "posix-umask [ -S ] [ mask ] [ prog... ]"
#define dieusage() strerr_dieusage(100, USAGE)
#define dieout() strerr_diefu1sys(111, "write to stdout")


 /* well, unlike posix-cd, at least this one was fun to write */

static inline int output (int sym)
{
  mode_t mode = umask(0) ;
  size_t m = 0 ;
  char fmt[18] ;
  if (sym)
  {
    unsigned int i = 3 ;
    while (i--)
    {
      unsigned int mask = ~(mode >> (3*i)) ;
      fmt[m++] = "ogu"[i] ; fmt[m++] = '=' ;
      if (mask & 4) fmt[m++] = 'r' ;
      if (mask & 2) fmt[m++] = 'w' ;
      if (mask & 1) fmt[m++] = 'x' ;
      if (i) fmt[m++] = ',' ;
    }
  }
  else m += uint0_ofmt(fmt, mode, 4) ;
  fmt[m++] = '\n' ;
  if (buffer_putflush(buffer_1, fmt, m) < 0) dieout() ;
  return 0 ;
}

static void diesyntax (char const *) gccattr_noreturn ;
static void diesyntax (char const *s)
{
  strerr_dief3x(101, "internal parsing error: bad ", s, ". Please submit a bug-report.") ;
}

static inline uint8_t cclass (char c)
{
 /* char tables may be more efficient, but this is way more readable */
  switch (c)
  {
    case 0 : return 0 ;
    case ',' : return 1 ;
    case '+' :
    case '-' :
    case '=' : return 2 ;
    case 'u' :
    case 'g' :
    case 'o' : return 3 ;
    case 'a' : return 4 ;
    case 'r' :
    case 'w' :
    case 'x' :
    case 'X' :
    case 's' :
    case 't' : return 5 ;
    default : return 6 ;
  }
}

static inline uint8_t who_value (char c)
{
  switch (c)
  {
    case 'u' : return 4 ;
    case 'g' : return 2 ;
    case 'o' : return 1 ;
    case 'a' :
    case '+' : /* shortcut for when who is empty */
    case '-' :
    case '=' : return 7 ;
    default : diesyntax("who") ;
  }
}

static inline uint8_t perm_value (char c)
{
  switch (c)
  {
    case 'r' : return 4 ;
    case 'w' : return 2 ;
    case 'x' :
    case 'X' : return 1 ;
    case 's' :
    case 't' : return 0 ;
    default : diesyntax("perm") ;
  }
}

static inline unsigned int parsemode (char const *s)
{
  static uint16_t const table[5][7] =
  {
    { 0x005, 0x000, 0x064, 0x021, 0x021, 0x006, 0x006 },
    { 0x005, 0x006, 0x042, 0x021, 0x021, 0x006, 0x006 },
    { 0x005, 0x200, 0x042, 0x083, 0x006, 0x104, 0x006 },
    { 0x805, 0xe00, 0xc42, 0x006, 0x006, 0x006, 0x006 },
    { 0x805, 0xe00, 0xc42, 0x006, 0x006, 0x104, 0x006 }
  } ;
  unsigned int oldmode = ~umask(0) ;
  uint8_t modes[3] = { oldmode & 7, (oldmode >> 3) & 7, (oldmode >> 6) & 7 } ;
  uint8_t who = 0 ;
  uint8_t perm = 0 ;
  uint8_t state = 0 ;
  char op = 0 ;
  while (state < 5)
  {
    char c = *s++ ;
    uint16_t what = table[state][cclass(c)] ;
    state = what & 7 ;
    if (what & 0x020) who |= who_value(c) ;
    if (what & 0x080) perm = modes[byte_chr("ogu", 3, c)] ;
    if (what & 0x100) perm |= perm_value(c) ;
    if (what & 0x800)
    {
      unsigned int i = 3 ;
      while (i--) if (who & (1 << i))
        switch (op)
        {
          case '-' : modes[i] &= ~perm ; break ;
          case '+' : modes[i] |= perm ; break ;
          case '=' : modes[i] = perm ; break ;
          default : diesyntax("op") ;
        }
    }
    if (what & 0x040) op = c ;
    if (what & 0x200) who = 0 ;
    if (what & 0x400) perm = 0 ;
  }
  if (state > 5) strerr_dief1x(1, "invalid mode string") ;
  return ((unsigned int)modes[2] << 6) | ((unsigned int)modes[1] << 3) | modes[0] ;
}

int main (int argc, char const **argv)
{
  int sym = 0 ;
  unsigned int mode ;
  PROG = "posix-umask" ;
  setlocale(LC_ALL, "") ;  /* totally supported, I swear */

  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "S", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'S' : sym = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) return output(sym) ;
  if (!uint0_oscan(argv[0], &mode)) mode = ~parsemode(argv[0]) ;
  umask(mode & 00777) ;
  xexec0(argv+1) ;
}
