# Kiln performance

Measured on the development machine (Ubuntu 24.04, GCC 13, Python 3.12). Each
number is the **minimum of 5 runs**, with the binaries and Python interleaved so
that machine load affects them evenly. Times include process start-up. The
machine is shared, so repeat runs vary by about 10 percent; treat differences
smaller than that as noise.

## Results

| benchmark | 2.16.0 tree-walker | 2.17.0 VM (default) | 2.17.0 `--tree` | Python 3.12 |
|-----------|-------------------:|--------------------:|----------------:|------------:|
| fib       | 71 ms              | 62 ms               | 85 ms           | 26 ms       |
| loop      | 152 ms             | 103 ms              | 173 ms          | 172 ms      |
| lists     | 65 ms              | 68 ms               | 83 ms           | 65 ms       |
| strings   | 126 ms             | 126 ms              | 127 ms          | 22 ms       |

Reading the table:

- `fib` (recursive calls) is about 1.15 to 1.5 times faster than 2.16.0 in
  repeated runs. The variance on this machine is large, so the range is the
  honest figure.
- `loop` (a `while` loop with arithmetic and assignments) is about 1.5 times
  faster.
- `lists` and `strings` are not faster. `lists` uses `for (let x in ...)` and
  method calls such as `append`; method calls still run in the tree-walker.
  `strings` is dominated by `s = s + x`, which copies the whole string each
  time (not yet changed).
- The target in the 2.17 plan was at least 2 times faster on calls and loops.
  **That target was not met.** The remaining cost is in the value and scope
  representation, described below.

## Speed pass in 2.19.0

Two changes, both measured on the same machine and method as above (minimum of
5 interleaved runs):

| benchmark | 2.18.0 | 2.19.0 | Python 3.12 |
|-----------|-------:|-------:|------------:|
| fib       | 50 ms  | 24 ms  | 25 ms       |
| loop      | 101 ms | 67 ms  | 175 ms      |
| lists     | 74 ms  | 39 ms  | 67 ms       |
| strings   | 171 ms | 170 ms | 25 ms       |

## Text building in 2.20.0

Texts carry their length and share a growing buffer when built by `+`. Measured
with the same method (minimum of 5 interleaved runs):

| benchmark | 2.19.0 | 2.20.0 | Python 3.12 |
|-----------|-------:|-------:|------------:|
| strings   | 170 ms | 3 ms   | 25 ms       |

Peak memory for the strings benchmark fell from about 198 MB to about 11 MB.

- **`Value` is 16 bytes, not 56.** The separate fields were replaced by one
  tagged union. A module's name and environment now sit in one small record
  behind the value. Every executor benefits, so `--tree` also got faster (its
  `fib` went from about 85 to 35 ms in the 2.19.0 measurements).
- **Call and loop scopes are freed.** A function whose body contains no
  function, lambda or class definition cannot be captured, so its scope is
  released when the call returns. Compiled `for` loops release their scope at
  exit by the same reasoning. Scopes of nested blocks, closures and loops that
  define names are still not freed.

Tested with the native suites, the VM-vs-tree differential check, and an
AddressSanitizer/UBSan build on closure, class, lambda, module and recursion
programs.

## Historical profiling notes

The profiling notes from the 2.17/2.19 development period are historical and
should not be read as a profile of the 2.21.0 runtime. At that time, profiling
pointed to VM dispatch and scope lookup as significant costs. Later releases
changed the value representation, scope lifetime handling and text storage,
so those old hotspots must be measured again before making claims about the
current runtime.

## Current measurement status

The tables above record the measurements available for the listed releases;
they are not a complete benchmark of every language feature. In particular,
they do not establish performance for dictionaries, method-heavy programs,
exception-heavy programs, networking, graphics or database workloads. Run the
provided scripts on the target machine and record the exact compiler, Python
version, operating system and repeat count when publishing new comparisons.
