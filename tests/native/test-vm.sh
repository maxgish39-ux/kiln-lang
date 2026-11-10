#!/bin/sh
# Tests for the bytecode VM (default executor) and its --tree / --disasm modes.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 "$ROOT/runtime/kiln.c" -lm -o "$TMP/kiln"
K="$TMP/kiln"

cat > "$TMP/control.kiln" <<'Kiln'
fun find_first(xs, target) {
    for (let x in xs) {
        if x == target { return x * 10 }
    }
    return -1
}
print(find_first([1, 2, 3], 2), find_first([1, 2, 3], 9))
fun last_value() { 5 }
print(last_value())
let i = 0
let found = 0
while i < 10 {
    i = i + 1
    if i % 2 == 0 { continue }
    if i > 7 { break }
    found = found + i
}
print(i, found)
let s = ""
for (let c in "abc") { s = s + c + "-" }
print(s)
let n = 0
while n < 3 {
    n = n + 1
    try {
        if n == 2 { break }
    } catch (e) { print("no") }
}
print("after try break", n)
for (let a in [1, 2]) {
    for (let b in [1, 2, 3]) {
        if b == 2 { continue }
        if a == 2 { break }
        print(a, b)
    }
}
print(false and 1 / 0, true or 1 / 0)
print(1 if 0 else 2)
Kiln
expected_control='20 -1
5
9 16
a-b-c-
after try break 2
1 1
1 3
false true
2'
out="$("$K" "$TMP/control.kiln")"
[ "$out" = "$expected_control" ] || { echo "VM control flow output wrong:"; echo "$out"; exit 1; }

# The VM and the tree-walker must agree on every program.
for f in "$TMP/control.kiln" "$ROOT"/examples/*.kiln; do
    a="$("$K" "$f" </dev/null 2>&1)" || ea=$?
    b="$("$K" --tree "$f" </dev/null 2>&1)" || eb=$?
    [ "${ea:-0}" = "${eb:-0}" ] || { echo "exit codes differ for $f"; exit 1; }
    [ "$a" = "$b" ] || { echo "VM and --tree output differ for $f"; exit 1; }
    ea=; eb=
done

# Closures keep their own captured variables, even when made in a loop.
cat > "$TMP/closure.kiln" <<'Kiln'
fun make(k) {
    let base = k * 100
    fun add(x) { return base + x }
    return add
}
let f1 = make(1)
let f2 = make(2)
print(f1(5), f2(5))
Kiln
[ "$("$K" "$TMP/closure.kiln")" = "105 205" ] || { echo "closure capture wrong"; exit 1; }

# Runaway recursion stops with a clean error, not a crash.
cat > "$TMP/recurse.kiln" <<'Kiln'
fun f(n) { return f(n + 1) }
f(0)
Kiln
set +e
"$K" "$TMP/recurse.kiln" >/dev/null 2>"$TMP/recurse.err"
code=$?
set -e
[ "$code" = "3" ] || { echo "recursion: expected exit 3, got $code"; exit 1; }
grep -q "maximum recursion depth" "$TMP/recurse.err" || { echo "recursion: wrong message"; cat "$TMP/recurse.err"; exit 1; }

# Deep recursion through try/finally frames works (the stack and try bounds are large enough).
cat > "$TMP/deep.kiln" <<'Kiln'
fun h(n) {
    try {
        if n >= 1200 { return 0 }
        return h(n + 1) + 1
    } finally {
        let z = 1
    }
}
print(h(0))
Kiln
[ "$("$K" "$TMP/deep.kiln")" = "1200" ] || { echo "deep try recursion wrong"; exit 1; }

# finally runs on normal exit, on return, and before the error is reported.
cat > "$TMP/finally.kiln" <<'Kiln'
fun g() {
    try {
        return "returned"
    } finally {
        print("finally on return")
    }
}
print(g())
try {
    print("body")
} finally {
    print("finally on normal exit")
}
Kiln
expected_finally='finally on return
returned
body
finally on normal exit'
[ "$("$K" "$TMP/finally.kiln")" = "$expected_finally" ] || { echo "finally ordering wrong"; exit 1; }

# Runtime errors report the line of the faulting instruction.
printf 'let a = 1\nlet b = 0\nprint("start")\na = a / b\n' > "$TMP/line.kiln"
set +e
msg="$("$K" "$TMP/line.kiln" 2>&1 >/dev/null)"
set -e
echo "$msg" | grep -q "(line 4): division by zero" || { echo "wrong error line: $msg"; exit 1; }

# --disasm prints bytecode without running the program.
dis="$("$K" --disasm "$ROOT/benchmarks/loop.kiln")"
echo "$dis" | grep -q "LOOP_BEGIN" || { echo "--disasm missing LOOP_BEGIN"; exit 1; }
echo "$dis" | grep -q "JF" || { echo "--disasm missing JF"; exit 1; }

echo "Native VM tests passed."
