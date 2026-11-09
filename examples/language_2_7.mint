# MINT 2.7.0 features: string escapes, else if, list methods, split, random
import "string" as s
import "math" as math

let grade = 85
if grade >= 90 {
    print("A")
} else if grade >= 80 {
    print("B")
} else {
    print("C")
}

let words = s.split("pear,apple,mango", ",")
words.sort()
print(words.join(" | "))

let scores = [72, 95, 88]
scores.reverse()
print("Top score: " + str(scores[0]))
print("Has 88? " + str(scores.contains(88)))

let dice = math.random(1, 6)
print("Dice roll is between 1 and 6: " + str(dice >= 1 and dice <= 6))
print("Column one\tColumn two")
