# Changelog

## 2.21.0 — UTF-8 text

- Text is counted and indexed by character, not byte. `len`, `string.length`,
  `text[i]`, `text[a:b]` and `for (let c in text)` all work on whole characters.
  Negative indexes count from the end, in characters.
- `string.index_of` returns a character position. This changes its result for
  text with non-ASCII characters; for ASCII text it is the same as before.
- `string.split(text, "", n)` splits into characters. `pad_left` and `pad_right`
  count characters, and the fill must be a single character.
- Escapes: `\u{1F600}` writes the character with that code point.
- Invalid UTF-8 bytes each count as one character, so older text still works.
- Speed: text that is plain ASCII keeps the fast paths. Text with non-ASCII
  characters is cached per buffer (its character count and the last position
  asked for), so walking through it with `len` and indexing stays fast. A
  20,000-character loop over accented text went from about 1.5 s to 10 ms.
- Not included: `string.upper` and `string.lower` change only ASCII letters, and
  character classes (letters, digits) are ASCII-only.
- Hardened native buffer growth with overflow and allocation-failure checks; HTTP response-body length is validated before copying.
- Version reported as `Kiln 2.21.0-native`.

## 2.20.0 — Faster text building

- Strings carry their length. A text value is a pointer plus a 32-bit length
  (texts up to 4 GB; longer text stops with an error). Each text buffer has a
  small header recording how much of it is in use and its capacity.
- `a + b` appends into spare space in the buffer when `a` is the newest text in
  it. Older values keep their own lengths, so they never change. When the
  buffer is full, the result is copied into a buffer with twice the room, so the
  total memory used by a build-up of text grows in proportion to its final size.
- Anything that needs a C string (file names, SQLite, the GUI, HTTP, regular
  expressions, JSON parsing) gets a NUL-terminated view from one helper, which
  returns the value's own buffer when it already ends there and copies otherwise.
- Comparison, search, slicing, indexing, `for ... in` over text, truthiness and
  printing use the stored length.
- Measured on `benchmarks/strings.kiln`: about 170 ms down to 3 ms, and peak
  memory from about 198 MB to about 11 MB. Other benchmarks are unchanged.
- Known limits: text operations still count bytes, not UTF-8 characters.
  Values that are copied to C strings still allocate a copy for each call when
  they are shorter than their buffer. Values in general are still not reclaimed;
  this release only removes the copies that text building made.
- Version reported as `Kiln 2.20.0-native`.

## 2.19.0 — Faster values and calls

- `Value` is 16 bytes instead of 56. Its fields are now one tagged union; a
  module's name and environment are kept in a small record behind the value.
- Function calls release their scope on return when the body cannot capture it
  (no `fun`, `lambda` or `class` inside). Compiled `for` loops release their
  scope at exit. Fewer allocations, and no leak for those scopes.
- Measured on the benchmarks (see `docs/PERFORMANCE.md`): `fib` about 2 times
  faster than 2.18.0, `lists` about 1.9 times, `loop` about 1.5 times. `strings`
  is unchanged, because text concatenation still copies on every `+`.
- Version reported as `Kiln 2.19.0-native`.

## 2.18.0 — Text features

- Raw strings `r"..."`: no escape processing, so `r"C:\new"` is the literal path.
- Triple-quoted strings `"""..."""`: may span lines and use the usual escapes.
- `string.split(text, separator, max_splits)`: the optional third argument limits the number of splits, as in Python. The empty separator splits into single characters and honours the limit too.
- `string.pad_left(text, width[, fill])` and `string.pad_right(text, width[, fill])`: pad to a width with one fill character (default a space). Text longer than the width is returned unchanged.
- Added `tests/native/test-text.sh`, included in `make test`.
- Not changed: text operations still count bytes, not UTF-8 characters. `index_of` returns a byte offset.
- Version reported as `Kiln 2.18.0-native`.

## 2.17.0 — Bytecode VM (default executor)

**Report.**

- *What works.* The VM is the default executor. `kiln --tree` runs the
  original tree-walker, and `kiln --disasm file.kiln` prints the bytecode of the
  top-level program. The compiler handles literals, names, arithmetic,
  comparisons, unary operators, `and`/`or` (short-circuit), `let` and
  assignment, calls to named functions, `if`, `while`, `for ... in` over lists,
  text and dictionaries (when the loop body defines no names and contains no
  closures), `break`, `continue`, `return`, and function and module bodies.
  Everything else runs through the same tree-walker code from inside the VM, so
  the two paths share one set of semantics. All native suites pass unchanged,
  and a differential check (VM and `--tree` give the same output and exit code)
  passes on every example and benchmark. A new suite, `tests/native/test-vm.sh`
  (in `make test`), covers control flow, closure capture, deep recursion,
  `finally` ordering, error line numbers and `--disasm`.
- *Bugs fixed on the way.* Nesting `try` more than 256 deep wrote past a fixed
  array and crashed; the limit is now 4096 and reports a clean error. Runaway
  recursion crashed with a segmentation fault; it now stops with
  `maximum recursion depth of 1500 exceeded` (exit 3). The main thread's C stack
  soft limit is raised to 512 MB when the system allows it, so deep recursion
  through the tree-walker also fits.
- *Speed.* Measured in `docs/PERFORMANCE.md`. Function calls are about 1.15 to
  1.5 times faster and loops about 1.5 times faster than 2.16.0. Lists and text
  building are not faster. **The 2-times target was not reached.**
- *Not done.* Shrinking `Value`, resolving locals to slots, a value stack for
  call frames, cheap text concatenation, interned dictionary keys, a garbage
  collector (memory is still never freed), method calls and `try` compiled
  to bytecode, and every language addition in the plan (triple-quoted and raw
  strings, UTF-8 text operations, inheritance, `match`, keyword and spread
  arguments, new text methods, time formatting, more benchmarks and sample
  programs). These remain for a later release.
- *Decisions.* Version 2.17.0, because the VM changes no syntax (3.0.0 is for a
  breaking syntax change). `--tree` is kept, because it costs about ten lines
  (one shared function, `run_body`). Compiled `for` loops use the reused-scope
  rule only; loops whose body makes closures or defines names stay in the
  tree-walker. `--disasm` prints the top-level program only, not nested
  functions.

**Changes.**

- Language and runtime: identifiers are interned in the lexer; scope lookup
  compares pointers first; calls check a recursion limit; `try` nesting limit
  raised to 4096 with a clean error.
- Added `src/vm.inc` (compiler, VM, disassembler) and `tests/native/test-vm.sh`.
  The operator code moved from `eval` into the shared helpers `unop` and
  `binop`, so both executors use the same code.
- `kiln --version` and the usage text report 2.17.0.
- Added `docs/PERFORMANCE.md`.

## 2.16.0 — Language features and speed

- Language: `**` power (right-associative); compound assignment `+= -= *= /= %=` on variables, list items and dictionary keys; unpacking `let a, b = ...` and `a, b = b, a`; conditional expressions `x if c else y`; `not in`; `finally` blocks (with or without `catch`); scientific notation in numbers (`1e3`, `2.5e-3`); `print` with any number of arguments; default parameter values (`fun f(a, b = 2)`).
- Built-in functions: `assert`, `enumerate`, `zip`, `min`, `max`, `abs`, `sum`. A user definition with the same name takes priority over the built-in.
- Behavior changes: arithmetic (`- * / // % **`) on non-numbers is an error instead of silently giving a wrong number; `==` compares lists and dictionaries by contents (it used to compare identity for lists); `<` and `>` and friends compare numbers or text and error on other types.
- Speed: operators are resolved to codes once when the program is read; calls to user functions skip built-in name checks; blocks and loops that define no names reuse the surrounding scope; scopes store their first four variables inline; names are not copied when defined; text concatenation copies each part once. On the benchmarks in `benchmarks/`, function calls are about 3 to 3.5 times faster than before, lists about 6 times faster, loops about 1.6 times, and text building about 2.5 times.
- Added `benchmarks/` with the four benchmark programs (Kiln and Python) and `benchmarks/run.sh`.
- Added `tests/native/test-language.sh`, included in `make test`.
- Version reported as `Kiln 2.16.0-native`.

## 2.15.0 — HTTPS, regex, slices, stats and matrix libraries

- HTTPS: `https://` requests run the system `curl` command, with arguments passed as a list (no shell). The response is parsed with the same code as plain HTTP. Without curl, an https request reports a clear error. The HTTP client also skips `100 Continue` replies.
- Added the native `regex` module (`matches`, `find`, `find_all`, `replace`, `split`) using the C library's POSIX regex.
- Language: slicing `text[a:b]` and `list[a:b]` (with negative and omitted bounds), floor division `//`, dictionary `keys()` and `values()`, and the global functions `sorted`, `round` and `bool`.
- Added libraries `stats` (mean, median, variance, stdev, minimum, maximum) and `matrix` (zeros, identity, shape, transpose, add, scale, multiply, dot).
- Test server gained HTTPS support. `tests/native/test-network.sh` now checks HTTPS with a trusted and an untrusted self-signed certificate (skipped if curl or openssl is missing). `tests/native/test-breadth.sh` covers the new language features and libraries.
- Fixed a test that used an incorrect expected value for a negative slice.
- Version reported as `Kiln 2.15.0-native`.

## 2.14.0 — Split runtime, HTTP module

- Split the runtime into sections under `src/` (lexer, values, parser, environment, calls, graphics, helpers, network, modules, interpreter, main). `runtime/kiln.c` includes them in order, so the build and commands are unchanged. The joined sections are byte-for-byte the previous single file.
- The Makefile rebuilds when a section file changes.
- Added the built-in `http` module: `get`, `post`, `request`, and `get_json`, with status, ok flag, body and lowercase response headers. Chunked responses are decoded.
- `https://` addresses are refused with a clear error, because this build has no TLS library.
- Added `tests/native/test-network.sh`, which starts a local test server (`tests/native/http_server.py`) and is included in `make test`. It is skipped if python3 is missing.
- Version reported as `Kiln 2.14.0-native`.

## 2.13.0 — Renamed to Kiln, libraries, module search path

- Renamed the language from MINT to **Kiln**: the command is `kiln`, source files end in `.kiln`, the runtime is `runtime/kiln.c`, and error messages say `Kiln`. Released name is a proposal; check it before publishing.
- Fixed file permissions: the launcher, setup script, helper scripts and test scripts are executable again.
- Fixed a duplicated changelog header.
- Fixed test expectations that still showed the old name.
- Imports now search: the script's folder, then each folder in `KILN_PATH` (colon-separated), then a `lib` folder beside the `kiln` executable. `import "text"` finds `lib/text.kiln`.
- Added Kiln-written libraries in `lib/`: `text`, `numbers`, `collections`, `csv` and `testing`.
- Added `tests/native/test-libraries.sh`, included in `make test`.
- Version reported as `Kiln 2.14.0-native`.

## v1.0 — 2026-11-09

- Set the public release version to v1.0.
- Rewrote the README as an end-user guide.
- Updated the native runtime version output to `Kiln 1.0-native`.


## 2.12.0 — f-strings and the in operator

- Added f-strings: `f"Hello {name}"`, with any expression inside `{ }`, `{{` and `}}` for literal braces, and `{expr:.N}` for N decimal places.
- Added the `in` operator for lists (equal item), dictionaries (key) and text (substring).
- Added `tests/native/test-strings-in.sh`, included in `make test`.
- Version reported as `Kiln 2.12.0-native`.

## 2.11.0 — try / catch / throw, editor syntax files

- Added `try { ... } catch (name) { ... }` in the native runtime. The catch variable is optional. Errors raised by the runtime and by `throw` are catchable, including those raised inside functions and methods.
- Added `throw value` to raise an error with a custom message.
- Runtime errors in method and argument checks now use the same path, so they can be caught too.
- Added `editors/kiln.nanorc` (nano) and `editors/micro/kiln.yaml` (micro) syntax highlighting.
- Added `tests/native/test-trycatch.sh`, included in `make test`.
- Version reported as `Kiln 2.11.0-native`.

## 2.10.0 — Native classes, runtime error line numbers, no while-loop cap

- Added `class` definitions to the native runtime: `init` constructors, fields set with `this.name = ...`, methods using `this`, and methods calling other methods.
- Instances support `obj.field`, `obj["field"]`, `obj.field = value` and `obj.method(args)`. Reading a method without calling it returns a bound method.
- `type()` reports `instance` and `class`; printing an instance shows `<Name instance>`.
- Added `tests/native/test-classes.sh`, included in `make test`.
- Runtime errors now report the source line, e.g. `Kiln runtime error (line 2): undefined name 'x'`, including inside functions and methods.
- Removed the 10-million-iteration limit on `while` loops.
- Fixed error messages for list and dictionary index errors that printed a literal `\n`.
- Version reported as `Kiln 2.10.0-native`.

- Runtime errors now report the source line, e.g. `Kiln runtime error (line 2): undefined name 'x'`. Errors inside functions report the line where they occurred.
- Removed the 10-million-iteration limit on `while` loops; long-running loops now run until the condition is false.

## 2.9.0 — Lambda expressions, closures, file lines, more math

- Added lambda expressions: `fun(x) => expression` and `fun(x) { ... }`, usable anywhere a value is expected.
- Lambdas capture their surrounding variables and can be returned from functions.
- Added `=>` token.
- Named `fun name(...)` statements are unchanged.
- Added `fs.lines(path)`, returning the lines of a text file.
- Added `math.clamp`, `math.log` and `math.exp`.
- Version reported as `Kiln 2.9.0-native`.
- Added `tests/native/test-lambdas.sh` and `examples/lambdas_2_9.kiln`.

## 2.8.0 — Chained methods, functional list methods, new modules

- Method calls work on any expression: `text.split(",").map(f).join(" ")`.
- String values support method syntax, e.g. `"  hi ".trim()`.
- Added list methods `map`, `filter`, `reduce`, `for_each`, `find`, `any`, `all`, `sum`, `min` and `max`.
- Added `json` module: `encode` and `decode`.
- Added `path` module: `join`, `basename`, `dirname`, `extension`.
- Added `time` module: `now` and `sleep`.
- Added `string.repeat`.
- Version reported as `Kiln 2.8.0-native`.
- Added `tests/native/test-modules.sh` and `examples/modules_2_8.kiln`.

## 2.7.0 — Language features

- Added string escape sequences (`\n`, `\t`, `\r`, `\0`, `\\`, `\"`).
- Added `else if` chains.
- Added list methods `sort`, `reverse`, `contains`, `index_of`, `join` and `clear`.
- Added `string.split`.
- Added `math.random()` and `math.random(low, high)`.
- Version reported as `Kiln 2.7.0-native`.
- Added `examples/language_2_7.kiln` and extended `tests/native/test-features.sh`.

## 2.6.1 — Easier installation and string/operator fixes

- Added `./setup.sh` (one command) and `./setup.sh --gui` for the optional raylib graphics support.
- Added `scripts/install-deps.sh` and `make deps` / `make deps-gui`, replacing manual apt package lists.
- `make raylib` now stops with a clear message when graphics libraries are missing.
- Added the `%` modulo operator with floored semantics.
- Added `string.starts_with`, `string.ends_with`, `string.index_of` and `string.replace`.
- Fixed `string.contains`, which always failed with an arity error.
- Fixed strict `-Werror` build failures from unchecked `fread` results; `make test` now passes.
- Added `tests/native/test-features.sh` and `examples/strings_and_modulo.kiln`.
- Updated `examples/lists.kiln` and `examples/dicts.kiln` to the native `for (let x in ...)` syntax.

## 2.0.0 — Native runtime foundation

- Added a native C runtime with its own lexer, recursive-descent parser, AST evaluator, scopes, functions, recursion and runtime errors.
- Changed `kiln1` to launch the native runtime; normal execution and syntax checking no longer invoke Python.
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

- Added native imports of user-written `.kiln` files using `import "path.kiln" as alias`.
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
- Added a first-run raylib source fetcher pinned to the official 6.2 release; `make raylib` fetches raylib into `third_party/raylib`, builds its static library, then links Kiln against it.
- Updated the launcher to prefer the locally built raylib static library when present, while retaining system `pkg-config` fallback.
- Expanded the native raylib test double and added a clickable-widget example.


## 2.5.0 — Installable native runtime

- Added standard `make install`, `make install-user`, and `make uninstall` targets.
- Installs a `kiln` command wrapper, native runtime, examples, and documentation.
- Supports configurable `PREFIX` and staged `DESTDIR` installs.
- Added native filesystem functions: `fs.mkdir`, `fs.is_dir`, `fs.size`, and `fs.copy`.
- Added `kiln --help` and updated native version reporting.
- Documented Git-based installation and update workflow.


## 2.6.0
- Added native `for-in` loops for lists, strings and dictionaries.
- Added native `range(stop)`, `range(start, stop)`, and `range(start, stop, step)`.
- Added `break` and `continue` loop control.
- Added iteration example and regression coverage.
- Added GitHub Actions CI for strict native compilation, tests and install smoke testing.
