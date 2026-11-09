# Native MINT modules

MINT 2.0's first standard modules are implemented by the native runtime in `runtime/mint.c` and imported by name:

| Import name | Purpose | Availability |
|---|---|---|
| `math` | Numeric functions | Built in |
| `string` | String helpers | Built in |
| `fs` | Local file operations | Built in |
| `system` | Process environment helpers | Built in |
| `json` | Encode and decode JSON | Built in |
| `path` | Join and split file paths | Built in |
| `time` | Clock and sleep | Built in |
| `sqlite` | Embedded SQL database | Optional; requires SQLite development files when building |

This directory documents the public module surface. The native runtime currently uses a built-in module registry rather than dynamically loading arbitrary `.so` files or `.mint` module files. A stable extension ABI is a future feature; do not assume that C modules compiled against this release are binary-compatible with later releases.


Native `fs` additions in MINT 2.6.0: `mkdir(path)`, `is_dir(path)`, `size(path)`, and `copy(source, destination)`. They return booleans for operations and `-1` for a missing path's size.
