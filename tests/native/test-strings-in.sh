#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/mint.c" -lm -o "$TMP/mint"
cat > "$TMP/fstrings.mint" <<'MINT'
let name = "Ada"
let age = 36
let price = 12.5
print(f"Hello, {name}!")
print(f"Next year {name} will be {age + 1}.")
print(f"Upper: {name.upper()} len {len(name)}")
print(f"Price: {price:.2f}")
print(f"Braces: {{literal}} and {name}")
print(f"Nested: {[1, 2, 3].map(fun(x) => x * 2).join(\"-\")}")
print(f"")
print("in list: " + str(2 in [1, 2, 3]) + " not: " + str(5 in [1, 2, 3]))
print("text: " + str("ell" in "hello") + " dict: " + str("k" in {"k": 1}))
MINT
expected='Hello, Ada!
Next year Ada will be 37.
Upper: ADA len 3
Price: 12.50
Braces: {literal} and Ada
Nested: 2-4-6

in list: true not: false
text: true dict: true'
actual="$("$TMP/mint" "$TMP/fstrings.mint")"
if [ "$actual" != "$expected" ]; then
    echo "f-string / in output mismatch" >&2
    printf 'expected:\n%s\nactual:\n%s\n' "$expected" "$actual" >&2
    exit 1
fi
printf 'print(f"oops {x")\n' > "$TMP/bad.mint"
if "$TMP/mint" "$TMP/bad.mint" 2>/dev/null; then echo "unclosed f-string should fail" >&2; exit 1; fi
echo "Native string formatting and in-operator tests passed."
