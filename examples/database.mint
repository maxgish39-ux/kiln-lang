# SQLite is an optional native MINT module.
import "sqlite" as sqlite
let db = sqlite.open("mint-demo.db")
db.exec("CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, body TEXT)")
db.exec("INSERT INTO notes (body) VALUES ('Hello from native MINT')")
print(db.scalar("SELECT body FROM notes ORDER BY id DESC LIMIT 1"))
print(db.last_id())
db.close()
