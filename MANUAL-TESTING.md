# Kiln 2.21.0 manual testing

Run these commands from the extracted project directory on Linux. The launcher builds the native C runtime when needed.

1. `./kiln1 --version` should print `Kiln 2.21.0-native`.
2. `./kiln1 --check examples/native_runtime.kiln` should parse the example successfully.
3. `./kiln1 examples/native_runtime.kiln` should run the native runtime example.
4. `make test` runs the native test suites, including VM, language, text, filesystem, networking, libraries and optional integrations available in the environment.
5. `make install-check` checks the install layout and runs an installed-program smoke test.
6. If SQLite development files are installed, `make sqlite` builds SQLite support; then run `./kiln1 examples/database.kiln`.

## Optional raylib GUI

A desktop session and the raylib build dependencies are required:

```bash
make raylib
./kiln1 examples/native_gui.kiln
```

The GUI test double (`tests/native/test-raylib-stub.sh`) checks the native binding calls without opening a real window. It does not replace a manual visual test on a desktop.

## Benchmarks

Run `./benchmarks/run.sh` and see [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md). For meaningful comparisons, use the same machine, compiler settings and benchmark scripts, and repeat runs under similar system load.
