# Dictionaries + new built-ins

let person = {
    "name": "MINT",
    "version": 0.4,
    "awesome": true
}

print(person["name"])
print(person["version"])
print(type(person))
print(len(person))

print("---- range ----")
for (let i in range(1, 6)) {
    print(i)
}

print("---- mixed ----")
let data = {
    "nums": [10, 20, 30],
    "label": "test"
}
print(data["nums"][1])
print(str(data["label"]) + " ok")
