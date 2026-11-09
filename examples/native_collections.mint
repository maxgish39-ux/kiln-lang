# Native MINT collection test
let numbers = [10, 20, 30]
print(numbers[0])
numbers[1] = 25
numbers.append(40)
print(numbers)
print(numbers.pop())
print(len(numbers))
let person = {"name": "MINT", "version": 2.1}
print(person["name"])
person["language"] = "native"
print(person)
print(person.get("missing"))
print(person.has("version"))
person.set("author", "ThePyguy")
print(person["author"])
print(type(numbers))
print(type(person))
