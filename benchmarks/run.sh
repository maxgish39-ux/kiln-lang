#!/bin/sh
# Times each benchmark on Kiln and, if available, on Python 3.
# Usage: benchmarks/run.sh [path-to-kiln]   (default: ./bin/kiln, then kiln on PATH)
set -eu
HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
KILN="${1:-}"
if [ -z "$KILN" ]; then
    if [ -x "$HERE/../bin/kiln" ]; then KILN="$HERE/../bin/kiln"; else KILN="kiln"; fi
fi
time_ms() {
    t=0
    for i in 1 2 3; do
        s=$(date +%s%N); "$@" >/dev/null 2>&1; e=$(date +%s%N)
        t=$((t + (e - s)))
    done
    echo "$((t / 3 / 1000000)) ms"
}
printf '%-10s %12s %12s\n' benchmark kiln python3
for b in fib loop lists strings; do
    k="$(time_ms "$KILN" "$HERE/$b.kiln")"
    if command -v python3 >/dev/null 2>&1; then p="$(time_ms python3 "$HERE/$b.py")"; else p="n/a"; fi
    printf '%-10s %12s %12s\n' "$b" "$k" "$p"
done
