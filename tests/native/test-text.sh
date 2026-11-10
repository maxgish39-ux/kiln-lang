#!/bin/sh
# Tests for raw strings, triple-quoted strings, split limits and padding.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 "$ROOT/runtime/kiln.c" -lm -o "$TMP/kiln"
cat > "$TMP/text.kiln" <<'Kiln'
import "string" as string
let raw = r"C:\new\path"
print(raw, len(raw))
print(r"no \t escape")
let block = """first line
second line"""
print(block)
print("tab\there")
print("a,b,c,d".split(",", 2))
print("a,b,c".split(","))
print("abc".split("", 2))
print(string.pad_left("7", 3, "0"), string.pad_right("ab", 5, "."), "|" + string.pad_left("toolong", 3) + "|")
print("x" == """x""")
Kiln
expected='C:\new\path 11
no \t escape
first line
second line
tab	here
[a, b, c,d]
[a, b, c]
[a, b, c]
007 ab... |toolong|
true'
out="$("$TMP/kiln" "$TMP/text.kiln")"
[ "$out" = "$expected" ] || { echo "text output wrong:"; echo "$out"; exit 1; }
# Older string values never change after appends; C-string functions see the right text.
cat > "$TMP/alias.kiln" <<'Kiln'
import "string" as string
import "fs" as fs
let a = "ab"
let b = a + "c"
let c = a + "X"
print(a, b, c)
let d = b + "d"
print(b, d, len(a), len(b), len(c), len(d))
let t = b
b = b + "!"
print(t, b, t == "abc", b == "abc!")
let s = ""
let snaps = []
let i = 0
while i < 5 {
    s = s + "ab"
    snaps.append(s)
    i = i + 1
}
print(snaps)
print(snaps[1] + "-", snaps[3] == "ababab", snaps[2][0], snaps[4][1])
print(snaps[4].split("b", 2), string.upper(snaps[2]), string.pad_left(snaps[1], 6, "."))
print("aba" in snaps[1], snaps[2].index_of("ba") if false else "skip")
print("x" + 1, 2 + "y", "n:" + str(3))
let dict = {}
dict.set(snaps[1], "one")
dict.set(snaps[2], "two")
print(dict.get("ab"), dict.get("abab"), dict.has("ababab"))
let w = "hello world"
let p = w[0:5] + "!"
let q = w[6:11] + "?"
print(p, q, w)
print(string.replace(snaps[2], "ab", "X"), snaps[2])
Kiln
expected_alias=$(cat <<'EXPECTED'
ab abc abX
abc abcd 2 3 3 4
abc abc! true true
[ab, abab, ababab, abababab, ababababab]
abab- false a b
[a, a, ababab] ABABAB ..abab
true skip
x1 2y n:3
null one true
hello! world? hello world
XXX ababab
EXPECTED
)
[ "$("$TMP/kiln" "$TMP/alias.kiln")" = "$expected_alias" ] || { echo "alias test wrong"; exit 1; }
# UTF-8: length, indexing, slicing, iteration and the string functions count characters.
cat > "$TMP/utf8.kiln" <<'Kiln'
import "string" as string
let word = "héllo wörld 🎉"
print(len(word), string.length(word), len("plain"))
print(word[1], word[7], word[len(word) - 1])
print(word[0:5], word[6:11], word[-1])
let chars = []
for (let c in "añ😀") { chars.append(c) }
print(chars, len(chars))
print(string.index_of(word, "wörld"), string.index_of(word, "zz"))
print("日本語".split("", 2), string.pad_left("é", 3, "·"), string.pad_right("ü", 3, "-") + "|")
print("\u{1F600} caf\u{E9}", "\u{41}")
print("é" == "é", "é" + "!", "😀" in "a😀b")
print(string.upper("abc"), "x" + str(1))
let s = ""
for (let i in range(5)) { s = s + "ö" }
print(s, len(s), s[4], s == "ööööö")
Kiln
expected_utf8=$(cat <<'EXPECTED'
13 13 5
é ö 🎉
héllo wörld 🎉
[a, ñ, 😀] 3
6 -1
[日, 本, 語] ··é ü--|
😀 café A
true é! true
ABC x1
ööööö 5 ö true
EXPECTED
)
[ "$("$TMP/kiln" "$TMP/utf8.kiln")" = "$expected_utf8" ] || { echo "utf8 test wrong"; exit 1; }
echo "Native text tests passed."
