#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 "$ROOT/runtime/mint.c" -lm -o "$TMP/mint"
cat > "$TMP/check.mint" <<'MINT'
import "math" as math
import "string" as text
fun fact(n) {
    if n <= 1 { return 1 }
    return n * fact(n - 1)
}
let i = 0
while i < 3 {
    print(i)
    i = i + 1
}
print(fact(5))
print(math.sqrt(81))
print(text.upper("mint"))
let nums = [10, 20, 30]
nums[1] = 25
nums.append(40)
print(nums[1])
print(nums.pop())
print(len(nums))
let user = {"name": "MINT", "level": 2}
user["level"] = 3
user.set("active", true)
print(user["name"])
print(user.get("level"))
print(user.has("active"))
print(type(nums))
print(type(user))
let total = 0
for (let n in range(1, 6)) {
    if n == 3 { continue }
    total = total + n
}
print(total)
let letters = "mint"
for (let ch in letters) { print(ch) }
let record = {"first": 1, "second": 2}
for (let key in record) { print(key) }
let stop_at = 0
while stop_at < 10 {
    stop_at = stop_at + 1
    if stop_at == 4 { break }
}
print(stop_at)
print(range(5))
print(range(5, 0, -2))
let outer = 0
for (let a in range(3)) {
    for (let b in range(3)) {
        if b == 1 { break }
    }
    outer = outer + 1
}
print(outer)
MINT
OUT="$("$TMP/mint" "$TMP/check.mint")"
EXPECTED="$(printf '0\n1\n2\n120\n9\nMINT\n25\n40\n3\nMINT\n3\ntrue\nlist\ndict\n12\nm\ni\nn\nt\nfirst\nsecond\n4\n[0, 1, 2, 3, 4]\n[5, 3, 1]\n3')"
[ "$OUT" = "$EXPECTED" ] || { printf 'Expected:\n%s\nGot:\n%s\n' "$EXPECTED" "$OUT" >&2; exit 1; }
"$TMP/mint" --check "$TMP/check.mint" | grep -q 'parsed by native MINT'
cat > "$TMP/helper.mint" <<'MINT'
let prefix = "module"
fun greet(name) {
    return prefix + ": " + name
}
MINT
cat > "$TMP/modules.mint" <<MINT
import "$TMP/helper.mint" as helper
print(helper.prefix)
print(helper.greet("works"))
MINT
MODULE_OUT="$("$TMP/mint" "$TMP/modules.mint")"
[ "$MODULE_OUT" = "$(printf 'module\nmodule: works')" ] || {
    printf 'Module import test failed:\n%s\n' "$MODULE_OUT" >&2
    exit 1
}
echo 'Native runtime tests passed.'

cat > "$TMP/bad-break.mint" <<'MINT'
fun invalid() { break }
for (let n in [1, 2]) { invalid() }
MINT
if "$TMP/mint" "$TMP/bad-break.mint" >/dev/null 2>&1; then
    echo 'break inside a function incorrectly escaped into its caller loop.' >&2
    exit 1
fi
