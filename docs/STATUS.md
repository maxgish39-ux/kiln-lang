# Kiln — Project Status

Current release: **2.21.0 — UTF-8 text**. See CHANGELOG for what is done and what is not.

The default `kiln1` runner is now a native C executable. Its tokenizer, parser, AST evaluator, functions, scopes and core modules are implemented in C. It does not start Python. The launcher can rebuild the runtime using a local C compiler if the included binary is missing or stale.

Native standard modules: `math`, `string`, `fs`, `system`. Native lists and dictionaries support literals, indexing and mutation. User-written `.kiln` modules can be imported by path; module functions retain their module globals. Optional raylib bindings provide native windows, drawing, input and clickable buttons. The first `make raylib` fetches the official 6.2 source into `third_party/raylib/` before compiling it. Optional native database module: `sqlite` (requires SQLite development files at build time).

The former Python implementation remains in `kiln.py` and `kiln/` for compatibility. It still supports language features not yet ported to the native runtime, such as inheritance, and the older Tk-based GUI. The native runtime has its own optional raylib graphics API and a basic button primitive, but not a full higher-level widget toolkit. The native runtime currently supports a smaller language subset; see the README for the exact boundary. Native source-to-executable compilation without a Python frontend is not implemented yet.

Validation for this release: native runtime compiled with C11; `examples/native_runtime.kiln` executed; native recursion, while loops, lists, dictionaries, native modules, math and string module tests passed; raylib API tests passed against a test double (actual window rendering was not available in this environment). Syntax-only mode passed; optional SQLite create/insert/select integration passed in the development environment.


## Linux installation

The Makefile provides `install`, `install-user`, and `uninstall`. The user install targets `~/.local/bin` and `~/.local/lib/kiln`, requiring no administrator privileges. A Git clone can be updated with `git pull` and reinstalled with `make install-user`.


The native runtime now supports `for (let item in iterable)` over lists, strings and dictionaries; `range()` with positive or negative steps; and `break` / `continue`. These features are implemented in C and do not use the legacy Python frontend. The GitHub Actions workflow runs a strict C build, native tests, and an installation smoke test on pushes and pull requests.
