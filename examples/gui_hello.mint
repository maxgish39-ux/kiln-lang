# A minimal native desktop window.
let app = window("Hello from MINT", 480, 300)
label(app, "My first MINT desktop app", 24, 24)

fun clicked() {
    print("The button was clicked!")
}

button(app, "Click me", 24, 70, clicked)
show(app)
