# MINT Programming Language

**MINT v1.0** is a general-purpose programming language with a C-based native runtime, built-in modules, and optional desktop graphics through raylib.

MINT is designed to be approachable to read and write while still giving you tools for building real programs. You write source files with the `.mint` extension and run them with the `mint` command.

> **Important:** The default `mint` runtime is native C. Python is not needed to run MINT programs. A legacy Python implementation remains in the repository for compatibility and features that have not yet been ported.

## What you can do with MINT

- Write functions, recursive functions, and lambda expressions.
- Use variables, strings, numbers, booleans, lists, and dictionaries.
- Build logic with `if`, `else if`, `else`, and `while`.
- Organize code into reusable modules.
- Create classes with constructors, fields, and methods.
- Handle runtime errors with `try`, `catch`, and `throw`.
- Work with files, JSON, paths, dates, environment variables, and math through native modules.
- Build graphical desktop programs with the optional raylib-powered `gui` module.
- Use the optional SQLite integration when built with SQLite development libraries.

## Quick start

These instructions target Linux Mint, Ubuntu, and Debian.

### 1. Get the source

```bash
git clone https://github.com/maxgish39-ux/Mint-lang.git
cd Mint-lang
```

### 2. Install

The setup script installs the required build tools and installs MINT for your user:

```bash
./setup.sh
```

The script may use `apt` to install system build dependencies. The MINT command and runtime are installed under your home directory, not system-wide.

If `mint` is not found after installation, add the user binary directory to your PATH:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### 3. Check the installation

```bash
mint --version
```

### 4. Run your first program

Create a file named `hello.mint`:

```mint
let name = "MINT"
print("Hello from " + name + "!")
```

Run it:

```bash
mint hello.mint
```

You can also run an included example from the project directory:

```bash
./mint1 examples/hello.mint
```

## A small language tour

### Variables and values

```mint
let language = "MINT"
let year = 2026
let ready = true

print(language)
print(year)
print(ready)
```

### Conditions and loops

```mint
let count = 1

while count <= 3 {
    print(count)
    count = count + 1
}

if count == 4 {
    print("Finished")
} else {
    print("Not finished")
}
```

### Functions

```mint
fun greet(name) {
    return "Hello, " + name
}

print(greet("there"))
```

### Lists and dictionaries

```mint
let scores = [10, 20, 30]
scores.append(40)
print(scores[0])
print(scores.length)

let profile = {"name": "MINT", "version": "1.0"}
print(profile["name"])
```

### Classes

```mint
class Person {
    fun init(name) {
        this.name = name
    }

    fun greet() {
        return "Hi, " + this.name
    }
}

let person = Person("Ada")
print(person.greet())
```

### Error handling

```mint
try {
    let values = [1, 2]
    print(values[10])
} catch (error) {
    print("Caught an error: " + error)
}
```

### Importing modules

```mint
import "math" as math
print(math.sqrt(144))
```

You can also import another `.mint` source file:

```mint
import "helpers.mint" as helpers
print(helpers.greet("MINT"))
```

## Built-in modules

The native runtime includes these modules:

| Module | What it provides |
| --- | --- |
| `math` | Common mathematical functions, constants, and random numbers |
| `string` | Text splitting, searching, replacing, and conversion helpers |
| `fs` | Reading, writing, appending, checking, and removing files |
| `json` | Encoding and decoding supported MINT values as JSON |
| `path` | Joining paths and extracting file names, directories, and extensions |
| `time` | Current Unix time and sleep |
| `system` | Current working directory and environment-variable access |
| `sqlite` | Optional SQLite database support when built with SQLite development files |
| `gui` | Optional raylib-powered window, drawing, and input functions |

For the full function list and more examples, see the project documentation and example programs.

## Optional graphics support

MINT can use [raylib](https://www.raylib.com/) for desktop windows, drawing, and input.

Install the graphics build dependencies on Linux Mint, Ubuntu, or Debian:

```bash
sudo apt install build-essential make curl libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev libxfixes-dev
```

Build the raylib integration:

```bash
make raylib
```

The first GUI build downloads the upstream raylib 6.2 source and builds it locally. An internet connection is required for that first download.

Run a GUI example:

```bash
./mint1 examples/native_gui.mint
```

GUI support is optional. Regular command-line MINT programs do not need raylib.

## Build from source

To build the native runtime:

```bash
make
```

Or run the native launcher directly:

```bash
./mint1 --version
./mint1 examples/native_runtime.mint
./mint1 --check examples/native_runtime.mint
```

A C compiler such as GCC or Clang is required to build the native runtime. The launcher can build the runtime from `runtime/mint.c` when needed.

## Install options

Install for your user:

```bash
make
make install-user
```

Install system-wide:

```bash
sudo make install
```

Uninstall a user installation:

```bash
make uninstall PREFIX="$HOME/.local"
```

## Project layout

- `runtime/mint.c` — native C runtime, tokenizer, parser, evaluator, and built-in modules
- `mint1` — native launcher
- `mint/` and `mint.py` — legacy Python implementation
- `examples/` — sample MINT programs
- `tests/` — language and native-runtime tests
- `docs/` — project status and changelog
- `scripts/` — setup, dependency, and launcher scripts
- `third_party/` — notes and integration support for external libraries

## Current limitations

MINT v1.0 is an early release. Some language features and libraries are still evolving. In particular:

- The native runtime does not yet implement every feature available in the legacy Python implementation.
- Class inheritance and private members are not implemented.
- Module caching, circular-import detection, and package search paths are not implemented.
- The SQLite integration is intentionally small and does not yet provide parameterized queries or full row/list results.
- The raylib graphics module is optional and requires a supported desktop graphics environment.

Check `docs/STATUS.md` and `docs/CHANGELOG.md` for implementation notes included with the source.

## Contributing and feedback

Bug reports, feature ideas, and contributions are welcome. When reporting a problem, include:

1. Your operating system.
2. The output of `mint --version`.
3. A small `.mint` program that reproduces the issue.
4. The full error message, if there is one.

Please test changes before submitting a pull request.

## License

Check the repository for its license file and the licenses of bundled or fetched third-party dependencies before redistributing builds.
