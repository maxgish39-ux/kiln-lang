# Lists, indexing, for loops, break/continue, len

let nums = [10, 20, 30, 40, 50]
print(len(nums))
print(nums[0])
print(nums[2])

print("---- for loop ----")
for (let n in nums) {
    if n == 30 {
        continue
    }
    if n == 50 {
        break
    }
    print(n)
}

print("---- nested ----")
let matrix = [[1, 2], [3, 4]]
print(matrix[1][0])
