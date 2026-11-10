CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra
RUNTIME = bin/kiln
SOURCE = runtime/kiln.c
RAYLIB_DIR = third_party/raylib
RAYLIB_SRC = $(RAYLIB_DIR)/src
RAYLIB_LIB = $(RAYLIB_SRC)/libraylib.a
RAYLIB_SYSTEM_LIBS ?= -lGL -lm -lpthread -ldl -lrt -lX11
SQLITE_CFLAGS := $(shell pkg-config --cflags sqlite3 2>/dev/null)
SQLITE_LIBS := $(shell pkg-config --libs sqlite3 2>/dev/null)
ifneq ($(strip $(SQLITE_LIBS)),)
SQLITE_DEFINE = -DKILN_WITH_SQLITE
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

$(RUNTIME): $(SOURCE) $(wildcard src/*.inc)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SOURCE) -lm -o $(RUNTIME)

fetch-raylib:
	./scripts/fetch-raylib.sh

raylib: fetch-raylib
	@pkg-config --exists x11 gl || { echo "Graphics libraries are missing. Run: ./setup.sh --gui" >&2; exit 1; }
	$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_DESKTOP
	mkdir -p bin
	$(CC) $(CFLAGS) -DKILN_WITH_RAYLIB $(SQLITE_DEFINE) -I$(RAYLIB_SRC) $(SOURCE) $(RAYLIB_LIB) $(RAYLIB_SYSTEM_LIBS) $(SQLITE_CFLAGS) $(SQLITE_LIBS) -o $(RUNTIME)

sqlite:
	@pkg-config --exists sqlite3 || { echo "SQLite development files not found." >&2; exit 1; }
	mkdir -p bin
	$(CC) $(CFLAGS) -DKILN_WITH_SQLITE $(SOURCE) -lm $$(pkg-config --cflags --libs sqlite3) -o $(RUNTIME)

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
	sh tests/native/test-libraries.sh
	sh tests/native/test-network.sh
	sh tests/native/test-breadth.sh
	sh tests/native/test-language.sh
	sh tests/native/test-modules.sh
	sh tests/native/test-lambdas.sh
	sh tests/native/test-classes.sh
	sh tests/native/test-trycatch.sh
	sh tests/native/test-strings-in.sh
	sh tests/native/test-vm.sh
	sh tests/native/test-text.sh

install: all
	@test -x $(RUNTIME) || { echo "Build failed: $(RUNTIME) missing" >&2; exit 1; }
	$(INSTALL) -d "$(DESTDIR)$(bindir)" "$(DESTDIR)$(libdir)/kiln" "$(DESTDIR)$(sharedir)/kiln/examples" "$(DESTDIR)$(sharedir)/doc/kiln"
	$(INSTALL) -m 755 $(RUNTIME) "$(DESTDIR)$(libdir)/kiln/kiln"
	$(INSTALL) -d "$(DESTDIR)$(libdir)/kiln/lib"
	cp lib/*.kiln "$(DESTDIR)$(libdir)/kiln/lib/"
	sed 's|@KILN_PREFIX@|$(prefix)|g' scripts/kiln-launcher.sh > "$(DESTDIR)$(bindir)/kiln"
	chmod 755 "$(DESTDIR)$(bindir)/kiln"
	cp -R examples/. "$(DESTDIR)$(sharedir)/kiln/examples/"
	$(INSTALL) -m 644 README.md "$(DESTDIR)$(sharedir)/doc/kiln/README.md"
	$(INSTALL) -m 644 docs/STATUS.md docs/CHANGELOG.md "$(DESTDIR)$(sharedir)/doc/kiln/"
	@echo "Kiln installed. Ensure $(bindir) is in PATH."

install-user:
	$(MAKE) install PREFIX="$(HOME)/.local"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/kiln" "$(DESTDIR)$(libdir)/kiln/kiln"
	rm -rf "$(DESTDIR)$(sharedir)/kiln" "$(DESTDIR)$(sharedir)/doc/kiln" "$(DESTDIR)$(libdir)/kiln"

install-check: all
	@tmp="$$(mktemp -d)"; \
	$(MAKE) install DESTDIR="$$tmp" PREFIX=/usr; \
	"$$tmp/usr/lib/kiln/kiln" --version; \
	"$$tmp/usr/lib/kiln/kiln" "$$tmp/usr/share/kiln/examples/hello.kiln"; \
	printf 'import "text" as text\nprint(text.title_case("library ok"))\n' > "$$tmp/libcheck.kiln"; \
	"$$tmp/usr/lib/kiln/kiln" "$$tmp/libcheck.kiln"; \
	rm -rf "$$tmp"

clean:
	rm -f $(RUNTIME) $(RAYLIB_LIB)
