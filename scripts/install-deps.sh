#!/bin/sh
# Installs the system packages MINT needs on Debian, Ubuntu and Linux Mint.
#
#   ./scripts/install-deps.sh          compiler, make, curl, pkg-config, SQLite headers
#   ./scripts/install-deps.sh --gui    also the graphics libraries for the raylib GUI
set -eu

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

if ! command -v apt-get >/dev/null 2>&1; then
    echo "MINT's dependency installer needs apt (Linux Mint, Ubuntu or Debian)." >&2
    echo "On other systems install: a C11 compiler, make, curl, pkg-config and SQLite headers." >&2
    exit 1
fi

PKGS="build-essential make curl pkg-config libsqlite3-dev"
if [ "$GUI" -eq 1 ]; then
    PKGS="$PKGS libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev libxfixes-dev"
fi

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

echo "MINT: installing packages: $PKGS"
$SUDO apt-get update
# shellcheck disable=SC2086
$SUDO apt-get install -y $PKGS
echo "MINT: system dependencies installed."
