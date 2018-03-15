REAL_TARGETS := \
cd \
elgetopt \
elgetpositionals \
emptyenv \
exec \
execlineb \
exit \
export \
fdblock \
forbacktickx \
forstdin \
forx \
getcwd \
ifelse \
ifte \
ifthenelse \
import \
loopwhilex \
multidefine \
multisubstitute \
piperw \
runblock \
shift \
trap \
tryexec \
umask \
unexport \
wait \
withstdinas

CHANGED_TARGETS := \
background \
backtick \
define \
dollarat \
elglob \
fdclose \
fdmove \
fdswap \
fdreserve \
foreground \
getpid \
heredoc \
homeof \
if \
importas \
pipeline \
redirfd

BIN_TARGETS := $(REAL_TARGETS) $(CHANGED_TARGETS)

LIBEXEC_TARGETS :=

LIB_DEFS := EXECLINE=execline

EXTRA_TARGETS := .built .installed '&' '`' '=' '$$@' '*' '>&-' '>&' '<>' ';' '!' '<<' '~' '&&' '$$' '|' '<'
