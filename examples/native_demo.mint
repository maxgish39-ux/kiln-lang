# This program is compiled to a native executable.
let name = "MINT"
let count = 1
while count <= 3 {
    print(name + " native build " + count)
    count = count + 1
}
if count == 4 {
    print("Compilation works!")
} else {
    print("Unexpected result")
}
