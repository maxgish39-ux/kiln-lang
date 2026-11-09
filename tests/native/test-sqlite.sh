#!/bin/sh
set -eu
command -v pkg-config >/dev/null 2>&1 || { echo 'SKIP: pkg-config unavailable'; exit 0; }
pkg-config --exists sqlite3 || { echo 'SKIP: SQLite development files unavailable'; exit 0; }
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -DMINT_WITH_SQLITE "$ROOT/runtime/mint.c" -lm $(pkg-config --cflags --libs sqlite3) -o "$TMP/mint"
cat > "$TMP/db.mint" <<'MINT'
import "sqlite" as sqlite
let db = sqlite.open("test.db")
db.exec("CREATE TABLE sample (value TEXT)")
db.exec("INSERT INTO sample VALUES ('native sqlite')")
print(db.scalar("SELECT value FROM sample"))
db.close()
MINT
OUT="$(cd "$TMP" && ./mint db.mint)"
[ "$OUT" = 'native sqlite' ] || { echo "SQLite test failed: $OUT" >&2; exit 1; }
echo 'Native SQLite module test passed.'
