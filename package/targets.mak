BIN_TARGETS := \
background \
backtick \
case \
define \
dollarat \
elgetopt \
elgetpositionals \
elglob \
eltest \
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


EXTRA_BINS := execline
EXTRA_TEMP := src/multicall/execline.c

multicall multicall-all: execline

multicall-strip: execline
	exec $(STRIP) -R .note -R .comment execline

multicall-install: $(DESTDIR)$(bindir)/execline
	for i in $(BIN_TARGETS) $(EXTRA_TARGETS) ; do ./tools/install.sh -l execline $(DESTDIR)$(bindir)/$$i ; done

multicall-global-links: $(DESTDIR)$(sproot)/command/execline

.PHONY: multicall multicall-all multicall-strip multicall-install multicall-global-links

src/multicall/execline.c: tools/gen-multicall.sh src/execline/deps-exe src/include/execline/config.h src/include/execline/execline.h
	./tools/gen-multicall.sh > src/multicall/execline.c
