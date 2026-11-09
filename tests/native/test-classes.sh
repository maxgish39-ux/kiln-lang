#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/mint.c" -lm -o "$TMP/mint"
cat > "$TMP/classes.mint" <<'MINT'
class Person {
    fun init(name, age) {
        this.name = name
        this.age = age
    }
    fun greet() {
        return "Hello, " + this.name
    }
    fun birthday() {
        this.age = this.age + 1
        return this.age
    }
    fun intro() {
        return this.greet() + " (" + str(this.age) + ")"
    }
}
class Counter {
    fun init() {
        this.count = 0
    }
    fun tick() {
        this.count = this.count + 1
        return this
    }
}
class Empty {
}
let a = Person("Ada", 36)
let b = Person("Linus", 20)
print(a.intro())
print(a.birthday())
print(a.age)
print(b.age)
b.name = "Linus T"
print(b.greet())
print(type(a))
print(a)
print(Person)
let c = Counter()
c.tick().tick().tick()
print(c.count)
print(c["count"])
let g = a.greet
print(g())
print(type(Empty()))
MINT
expected='Hello, Ada (36)
37
37
20
Hello, Linus T
instance
<Person instance>
<class Person>
3
3
Hello, Ada
instance'
actual="$("$TMP/mint" "$TMP/classes.mint")"
if [ "$actual" != "$expected" ]; then
    echo "class test output mismatch" >&2
    printf 'expected:\n%s\nactual:\n%s\n' "$expected" "$actual" >&2
    exit 1
fi
printf 'class P {\n fun init(x) { this.x = x }\n}\nlet p = P(1)\nprint(p.nope)\n' > "$TMP/bad.mint"
if "$TMP/mint" "$TMP/bad.mint" 2>"$TMP/err.txt"; then
    echo "expected a runtime error for a missing member" >&2
    exit 1
fi
grep -q "line 5" "$TMP/err.txt" || { echo "error did not report line 5" >&2; cat "$TMP/err.txt" >&2; exit 1; }
echo "Native class tests passed."
