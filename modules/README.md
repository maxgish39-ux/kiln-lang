# Native Kiln modules

Kiln 2.0's first standard modules are implemented by the native runtime in `runtime/kiln.c` and imported by name:

| Import name | Purpose | Availability |
|---|---|---|
| `math` | Numeric functions | Built in |
| `string` | String helpers | Built in |
| `fs` | Local file operations | Built in |
| `system` | Process environment helpers | Built in |
| `json` | Encode and decode JSON | Built in |
| `regex` | Regular expressions (POSIX extended syntax) | Built in |
| `http` | HTTP requests over http:// (GET, POST, custom requests, JSON responses) | Built in |
| `path` | Join and split file paths | Built in |
| `time` | Clock and sleep | Built in |
| `sqlite` | Embedded SQL database | Optional; requires SQLite development files when building |

This directory documents the public module surface. The native runtime currently uses a built-in module registry rather than dynamically loading arbitrary `.so` files or `.kiln` module files. A stable extension ABI is a future feature; do not assume that C modules compiled against this release are binary-compatible with later releases.


Native `fs` additions in Kiln 2.6.0: `mkdir(path)`, `is_dir(path)`, `size(path)`, and `copy(source, destination)`. They return booleans for operations and `-1` for a missing path's size.

Native `http` module, added in Kiln 2.14.0:

- `http.get(url)` returns a dictionary with `status` (number), `ok` (true for 2xx), `body` (text) and `headers` (a dictionary with lowercase names).
- `http.post(url, body)` sends `body` with `Content-Type: text/plain; charset=utf-8` unless you set another type.
- `http.request(method, url, body, headers)` sends any method. Pass `null` for no body or no headers.
- `http.get_json(url)` decodes a 2xx JSON response and raises an error otherwise.

`https://` URLs run the system `curl` command, which must be installed. Kiln passes arguments without a shell. Without curl, an https request reports a clear error.

Native `regex` module, added in Kiln 2.15.0 (POSIX extended syntax):

- `regex.matches(pattern, text)` returns true when the pattern matches anywhere in the text.
- `regex.find(pattern, text)` returns the first match, or `null`.
- `regex.find_all(pattern, text)` returns a list of all matches. Empty matches are skipped.
- `regex.replace(pattern, text, replacement)` replaces every match with the replacement text (no back-references).
- `regex.split(pattern, text)` splits the text at each match.
