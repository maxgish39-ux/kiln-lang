# MINT 2.6: iteration, range(), break and continue

let scores = [12, 18, 7, 20, 15]
let total = 0

for (let score in scores) {
    if score < 10 {
        continue
    }
    total = total + score
}

print("Total of passing scores:")
print(total)

print("Countdown:")
for (let number in range(5, 0, -1)) {
    print(number)
}

print("Stop at the first value above 15:")
for (let score in scores) {
    if score > 15 {
        break
    }
    print(score)
}
