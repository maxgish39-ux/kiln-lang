#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/mint.c" -lm -o "$TMP/mint"
cat > "$TMP/features.mint" <<'MINT'
import "string" as s

import "math" as math
print(s.contains("hello", "ell"))
print(s.starts_with("hello", "he"))
print(s.ends_with("hello", "lo"))
print(s.index_of("hello", "ll"))
print(s.replace("a-b-c", "-", "+"))
let parts = s.split("a,b,,c", ",")
print(parts.join("|"))
print(10 % 3)
print(-7 % 3)
let x = 2
if x == 1 { print("one") } else if x == 2 { print("two") } else { print("other") }
print("tab[\t]")
let nums = [3, 1, 2]
nums.sort()
print(nums.join(","))
nums.reverse()
print(nums.join(","))
print(nums.contains(2))
print(nums.index_of(9))
nums.clear()
print(len(nums))
let r = math.random(1, 6)
print(r >= 1 and r <= 6)
MINT
expected='true
true
true
2
a+b+c
a|b||c
1
2
two
tab[	]
1,2,3
3,2,1
true
-1
0
true'
actual="$("$TMP/mint" "$TMP/features.mint")"
if [ "$actual" != "$expected" ]; then
    echo "Feature test failed:" >&2
    echo "$actual" >&2
    exit 1
fi
echo "Native feature tests passed."
