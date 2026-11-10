#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/bin" "$TMP/work"
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/kiln.c" -lm -o "$TMP/bin/kiln"
ln -s "$ROOT/lib" "$TMP/lib"
cat > "$TMP/work/libs.kiln" <<'Kiln'
import "text" as text
import "numbers" as numbers
import "collections" as collections
import "csv" as csv
import "testing" as testing
testing.equal("pad", text.pad_left("7", 3, "0"), "007")
testing.equal("title", text.title_case("hello big world"), "Hello Big World")
testing.equal("reverse", text.reverse_text("kiln"), "nlik")
testing.equal("count", text.count_occurrences("banana", "an"), 2)
testing.equal("gcd", numbers.gcd(12, 18), 6)
testing.equal("lcm", numbers.lcm(4, 6), 12)
testing.equal("fact", numbers.factorial(5), 120)
testing.equal("fib", numbers.fibonacci(10), 55)
testing.check("prime", numbers.is_prime(97))
testing.equal("primes", numbers.primes_up_to(10), [2, 3, 5, 7])
testing.equal("digits", numbers.digit_sum(9875), 29)
testing.equal("unique", collections.unique([1, 2, 2, 3, 1]), [1, 2, 3])
testing.equal("flatten", collections.flatten([[1], [2, 3]]), [1, 2, 3])
testing.equal("chunk", collections.chunk([1, 2, 3, 4, 5], 2), [[1, 2], [3, 4], [5]])
testing.equal("range", collections.range_list(2, 5), [2, 3, 4])
testing.equal("zip", collections.zip_pairs([1, 2], ["a", "b"]), [[1, "a"], [2, "b"]])
testing.equal("parse", csv.parse("a,b\n\"x,y\",\"say \"\"hi\"\"\"\n"), [["a", "b"], ["x,y", "say \"hi\""]])
testing.equal("format", csv.format([["a", "x,y"]]), "a,\"x,y\"\n")
testing.check("summary", testing.summary())
Kiln
# 1. Found through the lib folder beside the executable (bin/../lib), from another directory.
out="$(cd "$TMP/work" && "$TMP/bin/kiln" libs.kiln)"
echo "$out" | grep -q "passed 18, failed 0" || { echo "library tests failed (lib beside executable):" >&2; echo "$out" >&2; exit 1; }
# 2. Found through KILN_PATH.
mkdir -p "$TMP/elsewhere"
out="$(cd "$TMP/work" && KILN_PATH="$ROOT/lib" "$TMP/bin/kiln" libs.kiln)"
echo "$out" | grep -q "passed 18, failed 0" || { echo "library tests failed (KILN_PATH):" >&2; exit 1; }
# 3. A missing library gives a clear error.
printf 'import "no_such_library" as x\n' > "$TMP/work/missing.kiln"
if "$TMP/bin/kiln" "$TMP/work/missing.kiln" 2>"$TMP/err.txt"; then echo "missing library should fail" >&2; exit 1; fi
grep -q "cannot open module 'no_such_library'" "$TMP/err.txt" || { echo "unexpected error text" >&2; cat "$TMP/err.txt" >&2; exit 1; }
echo "Native library tests passed."
