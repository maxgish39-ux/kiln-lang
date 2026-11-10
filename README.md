# Kiln Programming Language

**Kiln 2.21.0** is a general-purpose programming language with a C-based native runtime, built-in modules, and optional desktop graphics through raylib.

Kiln is designed to be approachable to read and write while still giving you tools for building real programs. You write source files with the `.kiln` extension and run them with the `kiln` command.

> **Important:** The default `kiln` runtime is native C. Python is not needed to run Kiln programs. A legacy Python implementation remains in the repository for compatibility and features that have not yet been ported.

## What you can do with Kiln

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

These instructions target Linux Kiln, Ubuntu, and Debian.

### 1. Get the source

```bash
git clone https://github.com/maxgish39-ux/Kiln-lang.git
cd Kiln-lang
```

### 2. Install

The setup script installs the required build tools and installs Kiln for your user:

```bash
./setup.sh
```

The script may use `apt` to install system build dependencies. The Kiln command and runtime are installed under your home directory, not system-wide.

If `kiln` is not found after installation, add the user binary directory to your PATH:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### 3. Check the installation

```bash
kiln --version
```

### 4. Run your first program

Create a file named `hello.kiln`:

```kiln
let name = "Kiln"
print("Hello from " + name + "!")
```

Run it:

```bash
kiln hello.kiln
```

You can also run an included example from the project directory:

```bash
./kiln1 examples/hello.kiln
```

## A small language tour

### Variables and values

```kiln
let language = "Kiln"
let year = 2026
let ready = true

print(language)
print(year)
print(ready)
```

### Conditions and loops

```kiln
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

```kiln
fun greet(name) {
    return "Hello, " + name
}

print(greet("there"))
```

### Lists and dictionaries

```kiln
let scores = [10, 20, 30]
scores.append(40)
print(scores[0])
print(scores.length)

let profile = {"name": "Kiln", "version": "1.0"}
print(profile["name"])
```

### Classes

```kiln
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

```kiln
try {
    let values = [1, 2]
    print(values[10])
} catch (error) {
    print("Caught an error: " + error)
}
```

### Importing modules

```kiln
import "math" as math
print(math.sqrt(144))
```

You can also import another `.kiln` source file:

```kiln
import "helpers.kiln" as helpers
print(helpers.greet("Kiln"))
```

### Text patterns

```kiln
import "regex" as regex

print(regex.matches("^[A-Z][a-z]+$", "Kiln"))          # true
print(regex.find_all("[0-9]+", "1 22 333"))              # [1, 22, 333]
print(regex.replace("[aeiou]", "banana", "*"))          # b*n*n*
print(regex.split(",[ ]*", "a, b,c"))                   # [a, b, c]
```

Patterns use POSIX extended syntax: `.`, `*`, `+`, `?`, `|`, `()`, `[]`, `^` and `$`. Write `[0-9]` instead of `\d`.

### Slices and everyday helpers

```kiln
print("hello world"[6:])     # world
print([10, 20, 30][:2])      # [10, 20]
print(-7 // 2)              # -4 (floor division)
print(sorted([3, 1, 2]))    # [1, 2, 3]
print(round(2.567, 2))      # 2.57
```

Dictionaries have `keys()` and `values()`.

### Text and characters

Text is counted by character, so non-English text behaves as you would expect:

```kiln
let word = "héllo 🎉"
print(len(word), word[1], word[-1])   # 7 é 🎉
for (let c in "añ") { print(c) }       # a, then ñ
print("\u{1F600}")                     # the emoji written by code point
```

### Text literals

```kiln
import "string" as string
let path = r"C:\new\folder"          # raw: backslashes are kept
let poem = """Line one
Line two"""                            # triple quotes span lines
print(path)
print("a,b,c,d".split(",", 2))         # at most two splits: [a, b, c,d]
print(string.pad_left("7", 3, "0"))    # 007
```

### More language features

```kiln
let a, b = 1, 2          # unpacking
a, b = b, a              # swap
let n = 3
n += 4                    # compound assignment: += -= *= /= %=
print(2 ** 3 ** 2)        # 512 (power is right-associative)
print(1e3, 2.5e-3)        # scientific notation
print(3 not in [1, 2])    # true
let label = "big" if n > 3 else "small"
print(label)

fun greet(name, greeting = "hello") {
    return greeting + ", " + name
}
print(greet("ada"))

try {
    risky()
} catch (e) {
    print("failed: " + e)
} finally {
    print("always runs")
}

print(max(3, 9, 4), min([5, 2, 8]), abs(-7), sum([1, 2, 3.5]))
print(enumerate(["a", "b"]))   # [[0, a], [1, b]]
print(zip([1, 2], ["x", "y"])) # [[1, x], [2, y]]
assert(a == 2, "a should be 2")
```

Arithmetic on text is an error (`1 - "a"`), and `+` joins text. `==` compares lists and dictionaries by their contents. A function you define with the same name as a built-in (for example `max`) takes priority over the built-in.

## Performance

Kiln's native runtime compiles supported code to bytecode and executes it on its VM. The project includes repeatable benchmark programs and Python comparison scripts in `benchmarks/`. Historical measurements document major improvements to function calls, loops, lists and text building; see [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) for version-by-version results and the measurement method.

Run the benchmarks on your own machine with:

```bash
make
./benchmarks/run.sh
```

Benchmark results depend on hardware, compiler, interpreter version, system load and the exact workload. Do not treat results from different machines as directly comparable.

Two flags help with debugging: `kiln --tree program.kiln` runs the original tree-walking evaluator (useful for comparing results), and `kiln --disasm program.kiln` prints the bytecode for the top-level program without running it.

### Web requests

```kiln
import "http" as http

let reply = http.get("http://example.com/")
print(str(reply["status"]) + " " + reply["body"])

let data = http.get_json("http://127.0.0.1:8000/api/status")
print(data["name"])
```

`http` supports GET, POST and custom requests (`http.request(method, url, body, headers)`), and decodes JSON responses with `http.get_json`. `https://` addresses work when the `curl` command is installed: Kiln runs it for those requests, so no TLS library is needed to build Kiln.

## Libraries

Kiln ships with libraries written in Kiln itself, in the `lib/` folder:

| Library | What it does |
|---|---|
| `text` | Padding, repeating, title case, word splitting, reversing, counting |
| `numbers` | Even/odd, gcd, lcm, factorial, Fibonacci, primes, digit sums |
| `collections` | Unique items, flatten, ranges, chunks, zipping, counting |
| `csv` | Parse and write comma-separated text with quoted fields |
| `stats` | Mean, median, variance, standard deviation, minimum and maximum |
| `matrix` | Matrices and vectors: zeros, identity, transpose, add, scale, multiply, dot |
| `testing` | `check`, `equal` and `summary` for writing small test scripts |

```kiln
import "text" as text
import "csv" as csv

print(text.title_case("hello big world"))
print(csv.format([["name", "note"], ["Ada", "likes, commas"]]))
```

Kiln finds a library by looking in the folder of your script, then in each folder listed in the `KILN_PATH` environment variable (separated by `:`), then in the `lib` folder beside the `kiln` executable. You can write your own libraries the same way: put `mylib.kiln` in your project and use `import "mylib" as mylib`.

## Built-in modules

The native runtime includes these modules:

| Module | What it provides |
| --- | --- |
| `math` | Common mathematical functions, constants, and random numbers |
| `string` | Text splitting, searching, replacing, and conversion helpers |
| `fs` | Reading, writing, appending, checking, and removing files |
| `json` | Encoding and decoding supported Kiln values as JSON |
| `path` | Joining paths and extracting file names, directories, and extensions |
| `time` | Current Unix time and sleep |
| `system` | Current working directory and environment-variable access |
| `sqlite` | Optional SQLite database support when built with SQLite development files |
| `gui` | Optional raylib-powered window, drawing, and input functions |

For the full function list and more examples, see the project documentation and example programs.

## Optional graphics support

Kiln can use [raylib](https://www.raylib.com/) for desktop windows, drawing, and input.

Install the graphics build dependencies on Linux Kiln, Ubuntu, or Debian:

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
./kiln1 examples/native_gui.kiln
```

GUI support is optional. Regular command-line Kiln programs do not need raylib.

## Build from source

To build the native runtime:

```bash
make
```

Or run the native launcher directly:

```bash
./kiln1 --version
./kiln1 examples/native_runtime.kiln
./kiln1 --check examples/native_runtime.kiln
```

A C compiler such as GCC or Clang is required to build the native runtime. The launcher can build the runtime from `runtime/kiln.c` when needed.

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

- `runtime/kiln.c` — native runtime entry point; it includes the sections in `src/` (lexer, parser, evaluator, modules, HTTP and more)
- `lib/` — libraries written in Kiln (`text`, `numbers`, `collections`, `csv`, `testing`, `stats`, `matrix`)
- `kiln1` — native launcher
- `kiln/` and `kiln.py` — legacy Python implementation
- `examples/` — sample Kiln programs (`examples/tk/` needs the Python implementation and Tkinter)
- `tests/` — language and native-runtime tests
- `docs/` — project status and changelog
- `scripts/` — setup, dependency, and launcher scripts
- `third_party/` — notes and integration support for external libraries

## Current limitations

Kiln 2.21.0 is actively evolving. The native runtime and legacy Python implementation do not have identical feature sets. Known limitations include:

- The native runtime does not yet implement every feature available in the legacy Python implementation.
- Class inheritance and private members are not implemented.
- Module caching, circular-import detection, and package search paths are not implemented.
- The SQLite integration is intentionally small and does not yet provide parameterized queries or full row/list results.
- The raylib graphics module is optional and requires a supported desktop graphics environment.

Check `docs/STATUS.md` and `docs/CHANGELOG.md` for implementation notes included with the source.

## Contributing and feedback

Bug reports, feature ideas, and contributions are welcome. When reporting a problem, include:

1. Your operating system.
2. The output of `kiln --version`.
3. A small `.kiln` program that reproduces the issue.
4. The full error message, if there is one.

Please test changes before submitting a pull request.

## License

Before redistributing Kiln, check the repository license and the licenses of third-party dependencies. (This source archive does not currently include a top-level `LICENSE` file.)
