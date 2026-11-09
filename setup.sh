#!/bin/sh
# One-command MINT setup for Linux Mint, Ubuntu and Debian.
#
#   ./setup.sh          install dependencies and MINT for the current user
#   ./setup.sh --gui    also build the raylib graphics support
#
# Nothing is installed system-wide except the apt packages listed in
# scripts/install-deps.sh. MINT itself goes to ~/.local.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
GUI=0
for arg in "$@"; do
    case "$arg" in
        --gui) GUI=1 ;;
        -h|--help)
            sed -n '2,6p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

if [ "$GUI" -eq 1 ]; then
    sh "$ROOT/scripts/install-deps.sh" --gui
else
    sh "$ROOT/scripts/install-deps.sh"
fi

make -C "$ROOT" install-user

if [ "$GUI" -eq 1 ]; then
    make -C "$ROOT" raylib
    echo "MINT: graphics support built. Try: mint examples/native_gui.mint"
fi

echo
echo "MINT is installed in ~/.local/bin."
case ":$PATH:" in
    *":$HOME/.local/bin:"*) echo "Run: mint --version" ;;
    *) echo "Add it to PATH once:  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc  (then open a new terminal)" ;;
esac
