# Native GUI example. Requires raylib and `make raylib`.
import "gui" as gui

gui.open(800, 450, "MINT native GUI")
gui.fps(60)

while not gui.should_close() {
    gui.begin()
    gui.clear(24, 32, 48)
    gui.text("Hello from MINT!", 40, 50, 32, 240, 245, 255)
    gui.rect(40, 110, 220, 90, 59, 130, 246)
    gui.circle(360, 155, 45, 255, 190, 80)

    if gui.key_pressed("space") {
        gui.text("SPACE pressed", 40, 240, 20, 120, 220, 140)
    }
    gui.end()
}

gui.close()
