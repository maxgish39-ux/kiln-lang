#!/bin/sh
# One-command Kiln setup for Linux Kiln, Ubuntu and Debian.
#
#   ./setup.sh          install dependencies and Kiln for the current user
#   ./setup.sh --gui    also build the raylib graphics support
#
# Nothing is installed system-wide except the apt packages listed in
# scripts/install-deps.sh. Kiln itself goes to ~/.local.
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
    echo "Kiln: graphics support built. Try: kiln examples/native_gui.kiln"
fi

echo
echo "Kiln is installed in ~/.local/bin."
case ":$PATH:" in
    *":$HOME/.local/bin:"*) echo "Run: kiln --version" ;;
    *) echo "Add it to PATH once:  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc  (then open a new terminal)" ;;
esac
