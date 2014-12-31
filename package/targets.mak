BIN_TARGETS := \
background \
backtick \
cd \
define \
dollarat \
elgetopt \
elgetpositionals \
elglob \
emptyenv \
exec \
execlineb \
exit \
export \
fdblock \
fdclose \
fdmove \
fdreserve \
forbacktickx \
foreground \
forx \
getpid \
heredoc \
homeof \
if \
ifelse \
ifte \
ifthenelse \
import \
importas \
loopwhilex \
multidefine \
multisubstitute \
pipeline \
piperw \
redirfd \
runblock \
shift \
tryexec \
umask \
unexport \
wait

SBIN_TARGETS :=
LIBEXEC_TARGETS :=

ifdef DO_SHARED
SHARED_LIBS := libexecline.so
endif

ifdef DO_STATIC
STATIC_LIBS := libexecline.a
endif