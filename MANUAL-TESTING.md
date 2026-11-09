# MINT 2.0 manual testing

Run these from the extracted project directory on Linux Mint.

1. `./mint1 --version` should print `MINT 2.0.0-native`.
2. `./mint1 --check examples/native_runtime.mint` should report that the file parsed by native MINT.
3. `./mint1 examples/native_runtime.mint` should print factorial, square-root and uppercase-string results.
4. `./tests/native/test.sh` should print `Native runtime tests passed.`
5. If built with SQLite, run `./mint1 examples/database.mint`; it should create a small local database and print a saved message.

The native runtime currently does not render GUI windows or graphics. Those are not tested by this release. The old Python GUI examples still use the legacy interpreter and Tkinter.


## Native raylib GUI (2.6.0)

This test requires raylib development files discoverable by `pkg-config` and a desktop session:

```bash
make raylib
./mint1 examples/native_gui.mint
```

Expected: a window titled "MINT native GUI" opens, draws a dark background, text, a blue rectangle and an amber circle. Press Space to display the SPACE pressed label; close the window to exit. This visual test was not run in the automated build environment. The automated `tests/native/test-raylib-stub.sh` checks the MINT-to-raylib call bridge against a test double, not actual graphics rendering.
