#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/mint.c" -lm -o "$TMP/mint"
cat > "$TMP/modules.mint" <<'MINT'
import "json" as json
import "path" as path
import "time" as time
fun double(x) { return x * 2 }
fun is_even(x) { return x % 2 == 0 }
fun add(a, b) { return a + b }
let nums = [1, 2, 3, 4, 5, 6]
print(nums.map(double).join(","))
print(nums.filter(is_even).join(","))
print(nums.reduce(add, 0))
print(nums.find(is_even))
print(nums.any(is_even))
print(nums.all(is_even))
print(nums.sum())
print(nums.min())
print(nums.max())
print("  shout  ".trim().upper())
print("ab".repeat(3))
let data = {"name": "MINT", "tags": ["fast", "small"], "version": 2.7, "ok": true, "none": null}
print(json.encode(data))
print(json.decode(json.encode(data))["tags"].join("/"))
print(path.join("/home/user", "mint"))
print(path.basename("/a/b/c.txt"))
print(path.dirname("/a/b/c.txt"))
print(path.extension("/a/b/c.txt"))
print(time.now() > 1000000)
MINT
expected='2,4,6,8,10,12
2,4,6
21
2
true
false
21
1
6
SHOUT
ababab
{"name":"MINT","tags":["fast","small"],"version":2.7,"ok":true,"none":null}
fast/small
/home/user/mint
c.txt
/a/b
.txt
true'
actual="$("$TMP/mint" "$TMP/modules.mint")"
if [ "$actual" != "$expected" ]; then
    echo "Module test failed:" >&2
    echo "$actual" >&2
    exit 1
fi
echo "Native module tests passed."
