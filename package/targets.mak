LIBEXEC_TARGETS :=

LIB_DEFS := EXECLINE=execline

ifeq ($(MULTICALL),1)

BIN_TARGETS := execline
CONTENTS := $(notdir $(wildcard src/execline/deps-exe/*))
BIN_SYMLINKS := cd umask $(CONTENTS)
EXTRA_TEMP := src/multicall/execline.c

define symlink_definition
SYMLINK_TARGET_$(1) := execline
endef
$(foreach name,$(BIN_SYMLINKS),$(eval $(call symlink_definition,$(name))))

src/multicall/execline.c: tools/gen-multicall.sh $(CONTENTS:%=src/$(package)/%.c)
	./tools/gen-multicall.sh > src/multicall/execline.c

src/multicall/execline.o: src/multicall/execline.c src/include/execline/config.h src/include/execline/execline.h

else

BIN_TARGETS := $(notdir $(wildcard src/execline/deps-exe/*))
BIN_SYMLINKS := cd umask

ifeq ($(PEDANTIC_POSIX),1)
SYMLINK_TARGET_cd := posix-cd
SYMLINK_TARGET_umask := posix-umask
else
SYMLINK_TARGET_cd := execline-cd
SYMLINK_TARGET_umask := execline-umask
endif

endif
