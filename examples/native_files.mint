import "fs" as fs
fs.write("mint-native-output.txt", "Created by MINT's native filesystem module.\n")
print(fs.read("mint-native-output.txt"))
print(fs.exists("mint-native-output.txt"))
