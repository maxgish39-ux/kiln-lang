#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/kiln.c" -lm -o "$TMP/kiln"
cat > "$TMP/lang.kiln" <<'KILN'
let x = 10
x += 5
x -= 3
x *= 2
x /= 4
print(x)
print(2 ** 3 ** 2)
print(-2 ** 2)
print(1, "two", 3.5)
fun greet(name, greeting = "hello", punct = "!") {
    return greeting + ", " + name + punct
}
print(greet("ada"))
print(greet("ada", "yo", "?"))
print([1, 2] == [1, 2], [1, 2] == [2, 1])
print(max(3, 9, 4), min([5, 2, 8]), abs(-7), sum([1, 2, 3.5]))
print(enumerate(["a", "b"]))
print(zip([1, 2, 3], ["x", "y"]))
fun max(a, b) { return "user max" }
print(max(1, 2))
print(3 not in [1, 2], 2 not in [1, 2])
let a = 1 if false else 2
print(a)
let p, q = 10, 20
p, q = q, p
print(p, q)
print(1e3, 2.5e-3)
try {
    throw "boom"
} catch (e) {
    print("caught " + e)
} finally {
    print("after catch")
}
fun early() {
    try {
        return "from try"
    } finally {
        print("cleanup")
    }
}
print(early())
KILN
EXPECTED='6
512
-4
1 two 3.5
hello, ada!
yo, ada?
true false
9 2 7 6.5
[[0, a], [1, b]]
[[1, x], [2, y]]
user max
true false
2
20 10
1000 0.0025
caught boom
after catch
cleanup
from try'
OUT="$("$TMP/kiln" "$TMP/lang.kiln")"
if [ "$OUT" != "$EXPECTED" ]; then
    echo "language test output mismatch" >&2
    printf 'expected:\n%s\ngot:\n%s\n' "$EXPECTED" "$OUT" >&2
    exit 1
fi
# Errors must be reported, not silently accepted.
printf 'print(1 - "a")\n' > "$TMP/bad.kiln"
if "$TMP/kiln" "$TMP/bad.kiln" 2>/dev/null; then echo "arithmetic on text should fail" >&2; exit 1; fi
printf 'fun f(a = 1, b) { return a }\n' > "$TMP/bad2.kiln"
if "$TMP/kiln" "$TMP/bad2.kiln" 2>/dev/null; then echo "a parameter without a default after one with a default should fail" >&2; exit 1; fi
echo "Native language feature tests passed."
