CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra
RUNTIME = bin/mint
SOURCE = runtime/mint.c
RAYLIB_DIR = third_party/raylib
RAYLIB_SRC = $(RAYLIB_DIR)/src
RAYLIB_LIB = $(RAYLIB_SRC)/libraylib.a
RAYLIB_SYSTEM_LIBS ?= -lGL -lm -lpthread -ldl -lrt -lX11
SQLITE_CFLAGS := $(shell pkg-config --cflags sqlite3 2>/dev/null)
SQLITE_LIBS := $(shell pkg-config --libs sqlite3 2>/dev/null)
ifneq ($(strip $(SQLITE_LIBS)),)
SQLITE_DEFINE = -DMINT_WITH_SQLITE
endif

prefix ?= /usr/local
ifneq ($(origin PREFIX),undefined)
prefix := $(PREFIX)
endif
bindir = $(prefix)/bin
libdir = $(prefix)/lib
sharedir = $(prefix)/share
INSTALL = install

.PHONY: all clean test raylib sqlite fetch-raylib install install-user uninstall install-check deps deps-gui setup

all: $(RUNTIME)

$(RUNTIME): $(SOURCE)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SOURCE) -lm -o $(RUNTIME)

fetch-raylib:
	./scripts/fetch-raylib.sh

raylib: fetch-raylib
	@pkg-config --exists x11 gl || { echo "Graphics libraries are missing. Run: ./setup.sh --gui" >&2; exit 1; }
	$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_DESKTOP
	mkdir -p bin
	$(CC) $(CFLAGS) -DMINT_WITH_RAYLIB $(SQLITE_DEFINE) -I$(RAYLIB_SRC) $(SOURCE) $(RAYLIB_LIB) $(RAYLIB_SYSTEM_LIBS) $(SQLITE_CFLAGS) $(SQLITE_LIBS) -o $(RUNTIME)

sqlite:
	@pkg-config --exists sqlite3 || { echo "SQLite development files not found." >&2; exit 1; }
	mkdir -p bin
	$(CC) $(CFLAGS) -DMINT_WITH_SQLITE $(SOURCE) -lm $$(pkg-config --cflags --libs sqlite3) -o $(RUNTIME)

deps:
	./scripts/install-deps.sh

deps-gui:
	./scripts/install-deps.sh --gui

setup:
	./setup.sh

test:
	sh tests/native/test.sh
	sh tests/native/test-sqlite.sh
	sh tests/native/test-raylib-stub.sh
	sh tests/native/test-filesystem.sh
	sh tests/native/test-features.sh
	sh tests/native/test-modules.sh
	sh tests/native/test-lambdas.sh
	sh tests/native/test-classes.sh
	sh tests/native/test-trycatch.sh
	sh tests/native/test-strings-in.sh

install: all
	@test -x $(RUNTIME) || { echo "Build failed: $(RUNTIME) missing" >&2; exit 1; }
	$(INSTALL) -d "$(DESTDIR)$(bindir)" "$(DESTDIR)$(libdir)/mint" "$(DESTDIR)$(sharedir)/mint/examples" "$(DESTDIR)$(sharedir)/doc/mint"
	$(INSTALL) -m 755 $(RUNTIME) "$(DESTDIR)$(libdir)/mint/mint"
	sed 's|@MINT_PREFIX@|$(prefix)|g' scripts/mint-launcher.sh > "$(DESTDIR)$(bindir)/mint"
	chmod 755 "$(DESTDIR)$(bindir)/mint"
	cp -R examples/. "$(DESTDIR)$(sharedir)/mint/examples/"
	$(INSTALL) -m 644 README.md "$(DESTDIR)$(sharedir)/doc/mint/README.md"
	$(INSTALL) -m 644 docs/STATUS.md docs/CHANGELOG.md "$(DESTDIR)$(sharedir)/doc/mint/"
	@echo "MINT installed. Ensure $(bindir) is in PATH."

install-user:
	$(MAKE) install PREFIX="$(HOME)/.local"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/mint" "$(DESTDIR)$(libdir)/mint/mint"
	rm -rf "$(DESTDIR)$(sharedir)/mint" "$(DESTDIR)$(sharedir)/doc/mint" "$(DESTDIR)$(libdir)/mint"

install-check: all
	@tmp="$$(mktemp -d)"; \
	$(MAKE) install DESTDIR="$$tmp" PREFIX=/usr; \
	"$$tmp/usr/lib/mint/mint" --version; \
	"$$tmp/usr/lib/mint/mint" "$$tmp/usr/share/mint/examples/hello.mint"; \
	rm -rf "$$tmp"

clean:
	rm -f $(RUNTIME) $(RAYLIB_LIB)
