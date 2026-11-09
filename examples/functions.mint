# Functions, return, scope, reassignment

fun add(a, b) {
    return a + b
}

fun factorial(n) {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

print(add(10, 20))
print(factorial(5))

let x = 100
x = x + 50
print(x)
