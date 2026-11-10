#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
cat > "$TMP/fs.kiln" <<'Kiln'
import "fs" as fs
print(fs.mkdir("folder"))
print(fs.is_dir("folder"))
print(fs.write("folder/source.txt", "native fs test"))
print(fs.size("folder/source.txt"))
print(fs.copy("folder/source.txt", "folder/copy.txt"))
print(fs.read("folder/copy.txt"))
Kiln
OUT="$(cd "$TMP" && "$ROOT/kiln1" fs.kiln)"
printf '%s\n' "$OUT" | grep -qx 'true'
printf '%s\n' "$OUT" | grep -qx '14'
printf '%s\n' "$OUT" | grep -qx 'native fs test'
printf '%s\n' "$OUT" | grep -qx 'true'
echo "Native filesystem module tests passed."
