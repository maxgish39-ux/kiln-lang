#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
VERSION="6.2"
DEST="$ROOT/third_party/raylib"
if [ -f "$DEST/src/raylib.h" ] && [ -f "$DEST/src/Makefile" ]; then
    echo "raylib $VERSION source already present: $DEST"
    exit 0
fi
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
ARCHIVE="$TMP/raylib.tar.gz"
URL="https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/$VERSION"
echo "Fetching raylib $VERSION source from the official raylib repository..."
if command -v curl >/dev/null 2>&1; then
    curl -fL --retry 3 --connect-timeout 15 "$URL" -o "$ARCHIVE"
elif command -v wget >/dev/null 2>&1; then
    wget -O "$ARCHIVE" "$URL"
else
    echo "Install curl or wget to fetch raylib source." >&2
    exit 1
fi
mkdir -p "$TMP/unpack"
tar -xzf "$ARCHIVE" -C "$TMP/unpack"
SRC="$(find "$TMP/unpack" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
[ -n "$SRC" ] && [ -f "$SRC/src/raylib.h" ] || { echo "Downloaded archive did not contain raylib source." >&2; exit 1; }
rm -rf "$DEST"
mkdir -p "$(dirname "$DEST")"
mv "$SRC" "$DEST"
echo "raylib $VERSION source is now vendored at third_party/raylib."
