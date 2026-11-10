"""Regression tests for Kiln 0.6."""
import contextlib
import io
import tempfile
import unittest
import sys
import types
from pathlib import Path
from unittest.mock import patch

from kiln.lexer import tokenize
from kiln.parser import Parser
from kiln.interpreter import Interpreter, KilnRuntimeError


def run_code(source, source_dir=None):
    statements = Parser(tokenize(source)).parse()
    output = io.StringIO()
    interpreter = Interpreter(source_dir=source_dir)
    with contextlib.redirect_stdout(output):
        interpreter.run(statements)
    return output.getvalue().strip().splitlines(), interpreter


class KilnLanguageTests(unittest.TestCase):
    def test_arithmetic_precedence(self):
        out, _ = run_code("print(10 + 5 * 2)\nprint((10 + 5) * 2)")
        self.assertEqual(out, ["20", "30"])

    def test_variables_and_reassignment(self):
        out, _ = run_code("let n = 1\nn = n + 1\nprint(n)")
        self.assertEqual(out, ["2"])

    def test_while_reassignment(self):
        out, _ = run_code("let i = 1\nwhile i <= 3 {\n print(i)\n i = i + 1\n}")
        self.assertEqual(out, ["1", "2", "3"])

    def test_block_scope(self):
        out, _ = run_code('let x = "outer"\nif true { let x = "inner" print(x) }\nprint(x)')
        self.assertEqual(out, ["inner", "outer"])

    def test_function_recursion(self):
        src = "fun fact(n) { if n <= 1 { return 1 } return n * fact(n - 1) }\nprint(fact(5))"
        out, _ = run_code(src)
        self.assertEqual(out, ["120"])

    def test_closure_mutates_captured_variable(self):
        src = '''fun counter() {
            let n = 0
            fun next() { n = n + 1 return n }
            return next
        }
        let c = counter()
        print(c())
        print(c())'''
        out, _ = run_code(src)
        self.assertEqual(out, ["1", "2"])

    def test_class_fields_methods_and_constructor(self):
        src = '''class Person {
            fun init(name) { this.name = name }
            fun greet() { return "Hello " + this.name }
        }
        let p = Person("Kiln")
        print(p.greet())
        p.name = "Kiln"
        print(p.name)'''
        out, _ = run_code(src)
        self.assertEqual(out, ["Hello Kiln", "Kiln"])

    def test_list_and_dictionary_mutation(self):
        src = 'let a = [1, 2]\na[0] = 9\nlet d = {"x": 1}\nd["x"] = 7\nprint(a[0])\nprint(d["x"])'
        out, _ = run_code(src)
        self.assertEqual(out, ["9", "7"])

    def test_for_string(self):
        out, _ = run_code('for ch in "kiln" { print(ch) }')
        self.assertEqual(out, list("kiln"))

    def test_file_builtins(self):
        with tempfile.TemporaryDirectory() as tmp:
            out, _ = run_code('write_file("note.txt", "hello")\nappend_file("note.txt", "!")\nprint(read_file("note.txt"))\nprint(file_exists("note.txt"))', tmp)
            self.assertEqual(out, ["hello!", "true"])

    def test_module_import_and_namespace(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "maths.kiln").write_text('let answer = 42\nfun double(n) { return n * 2 }', encoding="utf-8")
            source = 'import "maths.kiln" as maths\nprint(maths.answer)\nprint(maths.double(21))'
            out, _ = run_code(source, root)
            self.assertEqual(out, ["42", "42"])

    def test_circular_import_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a.kiln").write_text('import "b.kiln" as b', encoding="utf-8")
            (root / "b.kiln").write_text('import "a.kiln" as a', encoding="utf-8")
            with self.assertRaisesRegex(KilnRuntimeError, "Circular import"):
                run_code('import "a.kiln" as a', root)

    def test_undefined_variable_is_error(self):
        with self.assertRaises(KilnRuntimeError):
            run_code("print(missing)")

    def test_break_and_continue(self):
        src = '''let i = 0
        while i < 6 {
            i = i + 1
            if i == 2 { continue }
            if i == 5 { break }
            print(i)
        }'''
        out, _ = run_code(src)
        self.assertEqual(out, ["1", "3", "4"])


    def test_gui_builtins_are_registered_lazily(self):
        _, interpreter = run_code("print(1)")
        for name in ("window", "label", "button", "text_input", "get_value",
                     "set_value", "canvas", "draw_rect", "draw_circle",
                     "draw_line", "draw_text", "draw_image", "clear_canvas",
                     "show", "on_key", "on_click", "on_mouse_move",
                     "animate", "stop_animation", "close", "set_title"):
            value = interpreter.env.get(name)
            self.assertEqual(value[0], "builtin", name)

    def test_gui_examples_parse(self):
        examples = Path(__file__).resolve().parents[1] / "examples" / "tk"
        for filename in ("gui_hello.kiln", "graphics.kiln", "gui_input.kiln",
                         "interactive_canvas.kiln"):
            source = (examples / filename).read_text(encoding="utf-8")
            Parser(tokenize(source)).parse()

    def test_gui_bad_argument_count_is_reported_without_opening_tk(self):
        _, interpreter = run_code("print(1)")
        builtin = interpreter.env.get("window")
        with self.assertRaises(KilnRuntimeError):
            builtin[1]([])

    def test_gui_button_executes_kiln_callback(self):
        class FakeRoot:
            def __init__(self):
                self.callbacks = []
            def title(self, value): pass
            def geometry(self, value): pass
            def minsize(self, width, height): pass
            def mainloop(self):
                for callback in self.callbacks:
                    callback()
            def destroy(self): pass
            def bind(self, *args): pass
            def focus_force(self): pass
            def after(self, interval, callback): return "fake-after"
            def after_cancel(self, callback_id): pass

        class FakeWidget:
            def __init__(self, master=None, **kwargs):
                self.master = master
                self.kwargs = kwargs
                self.command = kwargs.get("command")
            def place(self, **kwargs): pass

        fake_tk = types.ModuleType("tkinter")
        fake_tk.Tk = FakeRoot
        fake_tk.Label = FakeWidget
        fake_tk.Entry = FakeWidget
        fake_tk.Canvas = FakeWidget

        def make_button(master, **kwargs):
            widget = FakeWidget(master, **kwargs)
            master.callbacks.append(widget.command)
            return widget

        fake_tk.Button = make_button
        source = '''
        fun clicked() { print("clicked from Kiln") }
        let app = window("Test", 320, 200)
        button(app, "Click", 10, 10, clicked)
        show(app)
        '''
        output = io.StringIO()
        with patch.dict(sys.modules, {"tkinter": fake_tk}):
            statements = Parser(tokenize(source)).parse()
            interpreter = Interpreter()
            with contextlib.redirect_stdout(output):
                interpreter.run(statements)
        self.assertEqual(output.getvalue().strip(), "clicked from Kiln")


    def test_native_compiler_generates_executable(self):
        import shutil, subprocess, tempfile
        from kiln.compiler import CCompiler
        if not (shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")):
            self.skipTest("No C compiler installed")
        source = "let x = 2\nwhile x < 5 {\n print(x)\n x = x + 1\n}\n"
        statements = Parser(tokenize(source)).parse()
        c_source = CCompiler().compile(statements)
        with tempfile.TemporaryDirectory() as tmp:
            cfile = Path(tmp) / "test.c"
            exe = Path(tmp) / "test-native"
            cfile.write_text(c_source)
            built = subprocess.run([shutil.which("cc") or shutil.which("gcc") or shutil.which("clang"),
                                    "-std=c11", str(cfile), "-o", str(exe)], capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stderr)
            ran = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(ran.stdout, "2\n3\n4\n")

    def test_native_functions_recursion_and_global_access(self):
        import shutil, subprocess, tempfile
        from kiln.compiler import CCompiler
        compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
        if not compiler:
            self.skipTest("No C compiler installed")
        source = """let offset = 2
fun factorial(n) {
    if n <= 1 { return 1 }
    return n * factorial(n - 1)
}
fun add_offset(n) { return n + offset }
print(factorial(5))
print(add_offset(40))
"""
        statements = Parser(tokenize(source)).parse()
        c_source = CCompiler().compile(statements)
        with tempfile.TemporaryDirectory() as tmp:
            cfile, exe = Path(tmp) / "functions.c", Path(tmp) / "functions"
            cfile.write_text(c_source, encoding="utf-8")
            built = subprocess.run([compiler, "-std=c11", str(cfile), "-o", str(exe)], capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stderr)
            ran = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(ran.returncode, 0, ran.stderr)
            self.assertEqual(ran.stdout, "120\n42\n")

    def test_native_compiler_reports_function_argument_mismatch(self):
        from kiln.compiler import CCompiler, NativeCompileError
        statements = Parser(tokenize("fun greet(name) { print(name) }\ngreet()" )).parse()
        with self.assertRaisesRegex(NativeCompileError, "expects 1 argument"):
            CCompiler().compile(statements)


if __name__ == "__main__":
    unittest.main(verbosity=2)
