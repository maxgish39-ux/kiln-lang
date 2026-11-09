# Changelog

## v1.0 — 2026-11-09

- Set the public release version to v1.0.
- Rewrote the README as an end-user guide.
- Updated the native runtime version output to `MINT 1.0-native`.

# Changelog

## 2.12.0 — f-strings and the in operator

- Added f-strings: `f"Hello {name}"`, with any expression inside `{ }`, `{{` and `}}` for literal braces, and `{expr:.N}` for N decimal places.
- Added the `in` operator for lists (equal item), dictionaries (key) and text (substring).
- Added `tests/native/test-strings-in.sh`, included in `make test`.
- Version reported as `MINT 2.12.0-native`.

## 2.11.0 — try / catch / throw, editor syntax files

- Added `try { ... } catch (name) { ... }` in the native runtime. The catch variable is optional. Errors raised by the runtime and by `throw` are catchable, including those raised inside functions and methods.
- Added `throw value` to raise an error with a custom message.
- Runtime errors in method and argument checks now use the same path, so they can be caught too.
- Added `editors/mint.nanorc` (nano) and `editors/micro/mint.yaml` (micro) syntax highlighting.
- Added `tests/native/test-trycatch.sh`, included in `make test`.
- Version reported as `MINT 2.11.0-native`.

## 2.10.0 — Native classes, runtime error line numbers, no while-loop cap

- Added `class` definitions to the native runtime: `init` constructors, fields set with `this.name = ...`, methods using `this`, and methods calling other methods.
- Instances support `obj.field`, `obj["field"]`, `obj.field = value` and `obj.method(args)`. Reading a method without calling it returns a bound method.
- `type()` reports `instance` and `class`; printing an instance shows `<Name instance>`.
- Added `tests/native/test-classes.sh`, included in `make test`.
- Runtime errors now report the source line, e.g. `MINT runtime error (line 2): undefined name 'x'`, including inside functions and methods.
- Removed the 10-million-iteration limit on `while` loops.
- Fixed error messages for list and dictionary index errors that printed a literal `\n`.
- Version reported as `MINT 2.10.0-native`.

- Runtime errors now report the source line, e.g. `MINT runtime error (line 2): undefined name 'x'`. Errors inside functions report the line where they occurred.
- Removed the 10-million-iteration limit on `while` loops; long-running loops now run until the condition is false.

## 2.9.0 — Lambda expressions, closures, file lines, more math

- Added lambda expressions: `fun(x) => expression` and `fun(x) { ... }`, usable anywhere a value is expected.
- Lambdas capture their surrounding variables and can be returned from functions.
- Added `=>` token.
- Named `fun name(...)` statements are unchanged.
- Added `fs.lines(path)`, returning the lines of a text file.
- Added `math.clamp`, `math.log` and `math.exp`.
- Version reported as `MINT 2.9.0-native`.
- Added `tests/native/test-lambdas.sh` and `examples/lambdas_2_9.mint`.

## 2.8.0 — Chained methods, functional list methods, new modules

- Method calls work on any expression: `text.split(",").map(f).join(" ")`.
- String values support method syntax, e.g. `"  hi ".trim()`.
- Added list methods `map`, `filter`, `reduce`, `for_each`, `find`, `any`, `all`, `sum`, `min` and `max`.
- Added `json` module: `encode` and `decode`.
- Added `path` module: `join`, `basename`, `dirname`, `extension`.
- Added `time` module: `now` and `sleep`.
- Added `string.repeat`.
- Version reported as `MINT 2.8.0-native`.
- Added `tests/native/test-modules.sh` and `examples/modules_2_8.mint`.

## 2.7.0 — Language features

- Added string escape sequences (`\n`, `\t`, `\r`, `\0`, `\\`, `\"`).
- Added `else if` chains.
- Added list methods `sort`, `reverse`, `contains`, `index_of`, `join` and `clear`.
- Added `string.split`.
- Added `math.random()` and `math.random(low, high)`.
- Version reported as `MINT 2.7.0-native`.
- Added `examples/language_2_7.mint` and extended `tests/native/test-features.sh`.

## 2.6.1 — Easier installation and string/operator fixes

- Added `./setup.sh` (one command) and `./setup.sh --gui` for the optional raylib graphics support.
- Added `scripts/install-deps.sh` and `make deps` / `make deps-gui`, replacing manual apt package lists.
- `make raylib` now stops with a clear message when graphics libraries are missing.
- Added the `%` modulo operator with floored semantics.
- Added `string.starts_with`, `string.ends_with`, `string.index_of` and `string.replace`.
- Fixed `string.contains`, which always failed with an arity error.
- Fixed strict `-Werror` build failures from unchecked `fread` results; `make test` now passes.
- Added `tests/native/test-features.sh` and `examples/strings_and_modulo.mint`.
- Updated `examples/lists.mint` and `examples/dicts.mint` to the native `for (let x in ...)` syntax.

## 2.0.0 — Native runtime foundation

- Added a native C runtime with its own lexer, recursive-descent parser, AST evaluator, scopes, functions, recursion and runtime errors.
- Changed `mint1` to launch the native runtime; normal execution and syntax checking no longer invoke Python.
- Added native `math`, `string`, `fs` and `system` modules.
- Added optional SQLite integration through the system SQLite library.
- Added native examples and a standalone C-runtime regression test.
- Kept the Python implementation as a compatibility path while remaining features are ported.

## 1.0.0 — Native compiler expansion

- Added top-level functions and recursion to the portable C compiler backend.
- Improved native compiler diagnostics and tests.

## 2.1.0 — Native collections

- Added native list literals, indexing, negative indexes, indexed assignment, `append`, `pop`, and `length`.
- Added native dictionary literals, indexing, indexed assignment, `get`, `set`, `has`, and `length`.
- Updated `len()` and `type()` for native collections.
- Added native collection regression tests and examples.
- Native execution remains independent of Python.
- Collections currently use process-lifetime allocation; garbage collection is not yet implemented.

## 2.2.0 — Native modules

- Added native imports of user-written `.mint` files using `import "path.mint" as alias`.
- Exposed module-level variables and functions through the alias.
- Added function closures so imported functions can read their module's own global variables.
- Added a native module import regression test.
- Module paths are currently resolved from the process working directory; module caching and circular-import detection are not implemented.

## 2.3.0 — Native GUI with raylib

- Added an optional native `gui` module backed by raylib.
- Added window lifecycle, frame begin/end, background clear, text, rectangle and circle drawing.
- Added keyboard and mouse input, mouse coordinates and target FPS.
- Added `make raylib` and launcher detection for a system-installed raylib pkg-config package.
- Added a raylib-compatible test double and regression coverage; no real window was rendered in the build environment.
- raylib remains a system dependency and is not bundled in the source archive.

## 2.4.0 — Expanded native GUI and self-vendoring raylib

- Added native GUI primitives: lines, rectangle outlines, text measurement, window dimensions, mouse wheel, held mouse buttons, title changes and fullscreen toggle.
- Added `gui.button(...)`, a clickable button primitive that returns `true` on the click frame.
- Added a first-run raylib source fetcher pinned to the official 6.2 release; `make raylib` fetches raylib into `third_party/raylib`, builds its static library, then links MINT against it.
- Updated the launcher to prefer the locally built raylib static library when present, while retaining system `pkg-config` fallback.
- Expanded the native raylib test double and added a clickable-widget example.


## 2.5.0 — Installable native runtime

- Added standard `make install`, `make install-user`, and `make uninstall` targets.
- Installs a `mint` command wrapper, native runtime, examples, and documentation.
- Supports configurable `PREFIX` and staged `DESTDIR` installs.
- Added native filesystem functions: `fs.mkdir`, `fs.is_dir`, `fs.size`, and `fs.copy`.
- Added `mint --help` and updated native version reporting.
- Documented Git-based installation and update workflow.


## 2.6.0
- Added native `for-in` loops for lists, strings and dictionaries.
- Added native `range(stop)`, `range(start, stop)`, and `range(start, stop, step)`.
- Added `break` and `continue` loop control.
- Added iteration example and regression coverage.
- Added GitHub Actions CI for strict native compilation, tests and install smoke testing.
