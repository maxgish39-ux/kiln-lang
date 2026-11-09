# Native function support: parameters, recursion and global reads.
let offset = 2

fun factorial(n) {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

fun add_offset(n) {
    return n + offset
}

print(factorial(5))
print(add_offset(40))
