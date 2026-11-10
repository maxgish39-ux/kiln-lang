#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/bin" "$TMP/work"
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/kiln.c" -lm -o "$TMP/bin/kiln"
ln -s "$ROOT/lib" "$TMP/lib"
cat > "$TMP/work/breadth.kiln" <<'KILN'
import "regex" as regex
import "stats" as stats
import "matrix" as matrix
let s = "hello world"
print(s[0:5] + "|" + s[6:] + "|" + s[-5:])
let xs = [10, 20, 30, 40]
print(xs[1:3])
print(7 // 2)
print(-7 // 2)
let d = {"a": 1, "b": 2}
print(d.keys())
print(d.values())
print(sorted([3, 1, 2]))
print(round(2.567, 2))
print(bool(0))
print(regex.matches("[0-9]+", "abc 42"))
print(regex.find("[0-9]+", "abc 42 and 7"))
print(regex.find_all("[0-9]+", "1 22 333"))
print(regex.replace("[aeiou]", "banana", "*"))
print(regex.replace("x*", "abc", "-"))
print(regex.split(",[ ]*", "a, b,c,   d"))
print(stats.mean([1, 2, 3, 4]))
print(stats.median([4, 1, 3, 2]))
print(stats.stdev([2, 4, 4, 4, 5, 5, 7, 9]))
print(matrix.multiply([[1, 2], [3, 4]], [[5, 6], [7, 8]]))
print(matrix.transpose([[1, 2, 3]]))
try {
    stats.mean([])
} catch (e) {
    print("caught: " + e)
}
KILN
OUT="$(cd "$TMP/work" && "$TMP/bin/kiln" breadth.kiln)"
EXPECTED="hello|world|world
[20, 30]
3
-4
[a, b]
[1, 2]
[1, 2, 3]
2.57
false
true
42
[1, 22, 333]
b*n*n*
-a-b-c-
[a, b, c, d]
2.5
2.5
2
[[19, 22], [43, 50]]
[[1], [2], [3]]
caught: stats.mean needs at least one number"
if [ "$OUT" != "$EXPECTED" ]; then
    echo "breadth test output mismatch" >&2
    printf 'expected:\n%s\ngot:\n%s\n' "$EXPECTED" "$OUT" >&2
    exit 1
fi
echo "Native language breadth tests passed."
