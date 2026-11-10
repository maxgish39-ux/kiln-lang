#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/kiln.c" -lm -o "$TMP/kiln"
cat > "$TMP/trycatch.kiln" <<'Kiln'
try {
    print(missing_name)
} catch (e) {
    print("caught: " + e)
}
fun risky(n) {
    if n > 2 { throw "too big: " + str(n) }
    return n * 10
}
fun safe(n) {
    try {
        return risky(n)
    } catch (e) {
        return "fallback (" + e + ")"
    }
}
print(safe(1))
print(safe(5))
let total = 0
let i = 0
while i < 6 {
    i = i + 1
    try {
        if i == 2 { continue }
        if i == 5 { break }
        let v = [1, 2][i]
        total = total + v
    } catch (e) {
        total = total + 100
    }
}
print("total " + str(total) + " i " + str(i))
try {
    try {
        throw "inner"
    } catch (e) {
        throw "rethrown " + e
    }
} catch (e) {
    print(e)
}
try {
    throw "no variable"
} catch {
    print("bare catch ok")
}
Kiln
expected="caught: undefined name 'missing_name'
10
fallback (too big: 5)
total 202 i 5
rethrown inner
bare catch ok"
actual="$("$TMP/kiln" "$TMP/trycatch.kiln")"
if [ "$actual" != "$expected" ]; then
    echo "try/catch output mismatch" >&2
    printf 'expected:\n%s\nactual:\n%s\n' "$expected" "$actual" >&2
    exit 1
fi
printf 'print("before")\nprint(nothere)\n' > "$TMP/uncaught.kiln"
if "$TMP/kiln" "$TMP/uncaught.kiln" >"$TMP/out.txt" 2>"$TMP/err.txt"; then
    echo "uncaught error should exit with failure" >&2
    exit 1
fi
grep -q "line 2" "$TMP/err.txt" || { echo "uncaught error did not report line 2" >&2; exit 1; }
echo "Native try/catch tests passed."
