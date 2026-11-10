#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O2 -Wall -Wextra -Werror -DKILN_WITH_RAYLIB \
  -I"$ROOT/tests/native/raylib_stub" \
  "$ROOT/runtime/kiln.c" "$ROOT/tests/native/raylib_stub/fake_raylib.c" \
  -lm -o "$TMP/kiln"
cat > "$TMP/gui.kiln" <<'Kiln'
import "gui" as gui
gui.open(640, 480, "test")
gui.fps(60)
while not gui.should_close() {
    gui.begin()
    gui.clear(1, 2, 3)
    gui.text("native", 10, 20, 24, 4, 5, 6)
    gui.rect(1, 2, 30, 40, 7, 8, 9)
    gui.circle(10, 20, 5, 11, 12, 13)
    print(gui.key_down("escape"))
    print(gui.key_pressed("space"))
    print(gui.mouse_x())
    print(gui.mouse_y())
    print(gui.mouse_pressed("left"))
    gui.line(1, 2, 3, 4, 14, 15, 16)
    gui.rect_outline(5, 6, 7, 8, 17, 18, 19)
    print(gui.measure_text("Kiln", 20))
    print(gui.window_width())
    print(gui.window_height())
    print(gui.mouse_wheel())
    print(gui.mouse_down("left"))
    gui.set_title("renamed")
    gui.toggle_fullscreen()
    print(gui.button("OK", 100, 200, 100, 80, 10, 20, 30, 240, 240, 240))
    gui.end()
}
gui.close()
Kiln
OUT="$("$TMP/kiln" "$TMP/gui.kiln")"
EXPECTED="$(printf 'OPEN:640:480:test\nFPS:60\nCLEAR:1,2,3\nTEXT:native:10:20:24:4,5,6\nRECT:1,2,30,40:7,8,9\nCIRCLE:10,20:5:11,12,13\ntrue\ntrue\n123\n234\ntrue\nLINE:1,2:3,4:14,15,16\nOUTLINE:5,6,7,8:17,18,19\n40\n800\n600\n1.25\ntrue\nTITLE:renamed\nFULLSCREEN\nRECT:100,200,100,80:35,45,55\nTEXT:OK:140:230:20:240,240,240\ntrue\nCLOSE')"
[ "$OUT" = "$EXPECTED" ] || {
    printf 'Expected:\\n%s\\nGot:\\n%s\\n' "$EXPECTED" "$OUT" >&2
    exit 1
}
echo 'Native raylib binding tests passed (test double; no real window rendered).'
