BIN_TARGETS := \
background \
backtick \
define \
dollarat \
elgetopt \
elgetpositionals \
elglob \
emptyenv \
envfile \
exec \
execlineb \
execline-cd \
execline-umask \
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
posix-cd \
posix-umask \
redirfd \
runblock \
shift \
trap \
tryexec \
unexport \
wait \
withstdinas

LIBEXEC_TARGETS :=

LIB_DEFS := EXECLINE=execline

ifeq ($(PEDANTIC_POSIX),1)
PEDANTIC_PREFIX := posix
else
PEDANTIC_PREFIX := execline
endif

EXTRA_TARGETS := cd umask

$(DESTDIR)$(bindir)/cd: $(DESTDIR)$(bindir)/$(PEDANTIC_PREFIX)-cd
	exec ./tools/install.sh -l $(PEDANTIC_PREFIX)-cd $(DESTDIR)$(bindir)/cd

$(DESTDIR)$(bindir)/umask: $(DESTDIR)$(bindir)/$(PEDANTIC_PREFIX)-umask
	exec ./tools/install.sh -l $(PEDANTIC_PREFIX)-umask $(DESTDIR)$(bindir)/umask
