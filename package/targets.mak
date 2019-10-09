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
envfile \
exec \
execlineb \
exit \
export \
fdblock \
fdclose \
fdmove \
fdswap \
fdreserve \
forbacktickx \
foreground \
forstdin \
forx \
getcwd \
getpid \
heredoc \
homeof \
if \
ifelse \
ifte \
ifthenelse \
importas \
loopwhilex \
multidefine \
multisubstitute \
pipeline \
piperw \
redirfd \
runblock \
shift \
trap \
tryexec \
umask \
unexport \
wait \
withstdinas

LIBEXEC_TARGETS :=

LIB_DEFS := EXECLINE=execline

ifeq ($(PEDANTIC_POSIX),1)

BIN_TARGETS += posix-cd

$(DESTDIR)$(bindir)/cd: $(DESTDIR)$(bindir)/posix-cd
	exec ./tools/install.sh -l posix-cd $(DESTDIR)$(bindir)/cd

endif
