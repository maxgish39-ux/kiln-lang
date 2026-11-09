# This program runs in the native MINT runtime. Python is not involved.
import "math" as math
import "string" as text
import "system" as sys

fun factorial(n) {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

let answer = factorial(6)
print("6! = " + str(answer))
print("sqrt(144) = " + str(math.sqrt(144)))
print(text.upper("mint native runtime"))
print("Working directory: " + sys.cwd())
