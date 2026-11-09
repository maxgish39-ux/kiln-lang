# Classes, constructors, instance fields and methods.
class Person {
    fun init(name) {
        this.name = name
    }

    fun greet() {
        return "Hello, " + this.name
    }
}

let person = Person("MINT")
print(person.greet())
person.name = "MINT 0.6"
print(person.name)
