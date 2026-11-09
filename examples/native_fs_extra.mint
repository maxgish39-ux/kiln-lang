import "fs" as fs

let folder = "mint_workspace"
print(fs.mkdir(folder))
print(fs.is_dir(folder))
print(fs.write(folder + "/note.txt", "Created by native MINT 2.6.0\n"))
print(fs.size(folder + "/note.txt"))
print(fs.copy(folder + "/note.txt", folder + "/note-copy.txt"))
print(fs.read(folder + "/note-copy.txt"))
