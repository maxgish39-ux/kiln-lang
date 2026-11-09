# Import and use a module written in MINT itself
import "examples/greeting_module.mint" as greetings
print(greetings.prefix)
print(greetings.greet("MINT"))
greetings.repeat_greeting("ThePyguy", 2)
