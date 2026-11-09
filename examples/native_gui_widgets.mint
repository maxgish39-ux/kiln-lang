# Native MINT GUI widget demo. Build with: make raylib
import "gui" as gui

let clicks = 0
gui.open(760, 460, "MINT 2.5 - Native GUI")
gui.fps(60)

while not gui.should_close() {
    gui.begin()
    gui.clear(18, 24, 38)
    gui.text("MINT GUI", 36, 32, 34, 238, 242, 250)
    gui.text("A native window, drawing API and clickable widgets", 38, 78, 18, 160, 174, 194)

    gui.rect(36, 124, 688, 230, 27, 36, 54)
    gui.rect_outline(36, 124, 688, 230, 57, 76, 102)
    gui.text("Button interaction", 60, 150, 22, 238, 242, 250)

    if gui.button("Click me", 60, 202, 190, 58, 59, 130, 246, 255, 255, 255) {
        clicks = clicks + 1
    }

    gui.text("Clicks: " + str(clicks), 60, 286, 24, 255, 190, 90)
    gui.text("Window: " + str(gui.window_width()) + " x " + str(gui.window_height()), 60, 390, 16, 160, 174, 194)
    gui.text("Press ESC to close", 530, 420, 14, 160, 174, 194)
    gui.end()
}

gui.close()
