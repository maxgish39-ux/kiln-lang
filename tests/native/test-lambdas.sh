#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/mint.c" -lm -o "$TMP/mint"
printf 'first line\nsecond\r\n\nlast' > "$TMP/input.txt"
cat > "$TMP/lambdas.mint" <<MINT
import "fs" as fs
import "math" as math
let nums = [1, 2, 3, 4, 5, 6]
print(nums.map(fun(x) => x * 10).join(","))
print(nums.filter(fun(x) => x % 2 == 0).join(","))
print(nums.reduce(fun(a, b) => a + b, 0))
let offset = 100
print(nums.map(fun(x) { return x + offset }).join(","))
let make_adder = fun(n) => fun(x) => x + n
let add5 = make_adder(5)
print(add5(10))
print((fun(a, b) => a * b)(6, 7))
print(nums.find(fun(x) => x > 4))
let lines = fs.lines("$TMP/input.txt")
print(len(lines))
print(lines.join("|"))
print(math.clamp(15, 0, 10))
print(math.exp(0))
MINT
expected='10,20,30,40,50,60
2,4,6
21
101,102,103,104,105,106
15
42
5
4
first line|second||last
10
1'
actual="$("$TMP/mint" "$TMP/lambdas.mint")"
if [ "$actual" != "$expected" ]; then
    echo "Lambda test failed:" >&2
    echo "$actual" >&2
    exit 1
fi
echo "Native lambda tests passed."
