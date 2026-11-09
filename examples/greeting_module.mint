# A reusable MINT module
let prefix = "Hello"
fun greet(name) {
    return prefix + ", " + name + "!"
}
fun repeat_greeting(name, count) {
    let i = 0
    while i < count {
        print(greet(name))
        i = i + 1
    }
}
