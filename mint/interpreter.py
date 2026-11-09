"""Tree-walking interpreter and runtime for MINT."""
from pathlib import Path
from .ast import (
    Number, String, Boolean, Var, BinOp, UnaryOp, Let, Assign, Print,
    If, While, For, Block, FunctionDef, FunctionCall, Return,
    ListLiteral, DictLiteral, Index, Break, Continue, ClassDef,
    GetAttr, SetAttr, SetIndex, Import,
)


class MintRuntimeError(Exception):
    """An error caused by a MINT program."""


class ReturnValue(Exception):
    def __init__(self, value): self.value = value
class BreakException(Exception): pass
class ContinueException(Exception): pass


class Environment:
    def __init__(self, parent=None):
        self.values = {}
        self.parent = parent

    def define(self, name, value):
        self.values[name] = value

    def get(self, name):
        if name in self.values: return self.values[name]
        if self.parent is not None: return self.parent.get(name)
        raise MintRuntimeError(f"Undefined variable '{name}'")

    def assign(self, name, value):
        if name in self.values:
            self.values[name] = value
            return
        if self.parent is not None:
            self.parent.assign(name, value)
            return
        raise MintRuntimeError(f"Cannot assign to undefined variable '{name}'")


class MintFunction:
    def __init__(self, declaration, closure):
        self.declaration, self.closure = declaration, closure
    def __repr__(self): return f"<fun {self.declaration.name}>"


class MintClass:
    def __init__(self, name, methods):
        self.name, self.methods = name, methods
    def find_method(self, name): return self.methods.get(name)
    def __repr__(self): return f"<class {self.name}>"


class MintInstance:
    def __init__(self, klass):
        self.klass = klass
        self.fields = {}
    def __repr__(self): return f"<{self.klass.name} instance>"


class BoundMethod:
    def __init__(self, function, instance):
        self.function, self.instance = function, instance


class MintModule:
    def __init__(self, path, environment):
        self.path, self.environment = str(path), environment


class Interpreter:
    def __init__(self, source_dir=None):
        self.globals = Environment()
        self.env = self.globals
        self.source_dir = Path(source_dir or Path.cwd()).resolve()
        self.module_cache = {}
        self.import_stack = []
        for name, fn in {
            "len": self._builtin_len, "str": self._builtin_str,
            "type": self._builtin_type, "range": self._builtin_range,
            "read_file": self._builtin_read_file,
            "write_file": self._builtin_write_file,
            "append_file": self._builtin_append_file,
            "file_exists": self._builtin_file_exists,
            "window": self._builtin_gui("window"),
            "label": self._builtin_gui("label"),
            "button": self._builtin_gui("button"),
            "text_input": self._builtin_gui("text_input"),
            "get_value": self._builtin_gui("get_value"),
            "set_value": self._builtin_gui("set_value"),
            "canvas": self._builtin_gui("canvas"),
            "draw_rect": self._builtin_gui("draw_rect"),
            "draw_circle": self._builtin_gui("draw_circle"),
            "draw_line": self._builtin_gui("draw_line"),
            "draw_text": self._builtin_gui("draw_text"),
            "draw_image": self._builtin_gui("draw_image"),
            "clear_canvas": self._builtin_gui("clear_canvas"),
            "show": self._builtin_gui("show"),
            "on_key": self._builtin_gui("on_key"),
            "on_click": self._builtin_gui("on_click"),
            "on_mouse_move": self._builtin_gui("on_mouse_move"),
            "animate": self._builtin_gui("animate"),
            "stop_animation": self._builtin_gui("stop_animation"),
            "close": self._builtin_gui("close"),
            "set_title": self._builtin_gui("set_title"),
        }.items():
            self.globals.define(name, ("builtin", fn))

    def _builtin_gui(self, name):
        def invoke(args):
            try:
                from .gui import dispatch
                return dispatch(self, name, args)
            except Exception as exc:
                if isinstance(exc, MintRuntimeError):
                    raise
                raise MintRuntimeError(str(exc)) from exc
        return invoke

    def _arity(self, name, args, count):
        if len(args) != count: raise MintRuntimeError(f"{name}() takes exactly {count} argument(s), got {len(args)}")

    def _builtin_len(self, args):
        self._arity("len", args, 1)
        if isinstance(args[0], (list, str, dict)): return len(args[0])
        raise MintRuntimeError(f"len() not supported for type {self.type_name(args[0])}")

    def _builtin_str(self, args):
        self._arity("str", args, 1)
        return self.stringify(args[0])

    def _builtin_type(self, args):
        self._arity("type", args, 1)
        return self.type_name(args[0])

    def _builtin_range(self, args):
        if not 1 <= len(args) <= 3: raise MintRuntimeError("range() takes 1 to 3 arguments")
        try: return list(range(*(int(v) for v in args)))
        except (TypeError, ValueError) as exc: raise MintRuntimeError(f"Invalid range arguments: {exc}") from exc

    def _builtin_read_file(self, args):
        self._arity("read_file", args, 1)
        path = self.resolve_path(args[0])
        try: return path.read_text(encoding="utf-8")
        except OSError as exc: raise MintRuntimeError(f"Cannot read '{path}': {exc}") from exc

    def _builtin_write_file(self, args):
        self._arity("write_file", args, 2)
        path = self.resolve_path(args[0])
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(self.stringify(args[1]), encoding="utf-8")
            return True
        except OSError as exc: raise MintRuntimeError(f"Cannot write '{path}': {exc}") from exc

    def _builtin_append_file(self, args):
        self._arity("append_file", args, 2)
        path = self.resolve_path(args[0])
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("a", encoding="utf-8") as stream: stream.write(self.stringify(args[1]))
            return True
        except OSError as exc: raise MintRuntimeError(f"Cannot append to '{path}': {exc}") from exc

    def _builtin_file_exists(self, args):
        self._arity("file_exists", args, 1)
        return self.resolve_path(args[0]).is_file()

    def resolve_path(self, value):
        if not isinstance(value, str): raise MintRuntimeError("File path must be a string")
        path = Path(value)
        return (path if path.is_absolute() else self.source_dir / path).resolve()

    def type_name(self, value):
        if isinstance(value, bool): return "bool"
        if isinstance(value, int): return "int"
        if isinstance(value, float): return "float"
        if isinstance(value, str): return "string"
        if isinstance(value, list): return "list"
        if isinstance(value, dict): return "dict"
        if isinstance(value, (MintFunction, BoundMethod)): return "function"
        if isinstance(value, MintClass): return "class"
        if isinstance(value, MintInstance): return "instance"
        if isinstance(value, MintModule): return "module"
        if value is None: return "null"
        return type(value).__name__

    def stringify(self, value):
        if value is True: return "true"
        if value is False: return "false"
        if value is None: return "null"
        return str(value)

    def is_truthy(self, value): return value is not None and value is not False

    def call_function(self, function, args, receiver=None):
        if isinstance(function, BoundMethod):
            receiver, function = function.instance, function.function
        if not isinstance(function, MintFunction):
            raise MintRuntimeError(f"Can only call functions, got {self.type_name(function)}")
        decl = function.declaration
        expected = len(decl.params)
        if len(args) != expected:
            raise MintRuntimeError(f"Function '{decl.name}' expected {expected} arguments, got {len(args)}")
        local = Environment(function.closure)
        if receiver is not None: local.define("this", receiver)
        for param, arg in zip(decl.params, args): local.define(param, arg)
        try: self.execute_block(decl.body, local)
        except ReturnValue as ret: return ret.value
        return None

    def get_attribute(self, obj, name):
        if isinstance(obj, MintInstance):
            if name in obj.fields: return obj.fields[name]
            method = obj.klass.find_method(name)
            if method is not None: return BoundMethod(method, obj)
            raise MintRuntimeError(f"'{obj.klass.name}' has no attribute '{name}'")
        if isinstance(obj, MintModule):
            try: return obj.environment.get(name)
            except MintRuntimeError: raise MintRuntimeError(f"Module '{obj.path}' has no exported name '{name}'")
        if isinstance(obj, MintClass):
            method = obj.find_method(name)
            if method is not None: return method
        raise MintRuntimeError(f"Type {self.type_name(obj)} has no attribute '{name}'")

    def import_module(self, relative_path):
        path = self.resolve_path(relative_path)
        if not path.is_file(): raise MintRuntimeError(f"Module not found: {path}")
        key = str(path)
        if key in self.import_stack: raise MintRuntimeError(f"Circular import detected: {path.name}")
        if key in self.module_cache: return self.module_cache[key]
        try:
            from .lexer import tokenize
            from .parser import Parser
            source = path.read_text(encoding="utf-8")
            statements = Parser(tokenize(source)).parse()
        except (OSError, SyntaxError) as exc:
            raise MintRuntimeError(f"Could not load module '{path.name}': {exc}") from exc
        module_env = Environment(self.globals)
        module = MintModule(path, module_env)
        self.module_cache[key] = module
        self.import_stack.append(key)
        previous_env, previous_dir = self.env, self.source_dir
        try:
            self.env = module_env
            self.source_dir = path.parent
            for statement in statements: self.eval(statement)
        except Exception:
            self.module_cache.pop(key, None)
            raise
        finally:
            self.env, self.source_dir = previous_env, previous_dir
            self.import_stack.pop()
        return module

    def eval(self, node):
        if isinstance(node, (Number, String, Boolean)): return node.value
        if isinstance(node, Var): return self.env.get(node.name)
        if isinstance(node, UnaryOp):
            value = self.eval(node.operand)
            try:
                if node.op == "not": return not self.is_truthy(value)
                if node.op == "-": return -value
                if node.op == "+": return +value
            except TypeError as exc: raise MintRuntimeError(f"Invalid operand for unary '{node.op}': {exc}") from exc
        if isinstance(node, BinOp):
            if node.op == "or":
                left = self.eval(node.left); return left if self.is_truthy(left) else self.eval(node.right)
            if node.op == "and":
                left = self.eval(node.left); return self.eval(node.right) if self.is_truthy(left) else left
            left, right = self.eval(node.left), self.eval(node.right)
            try:
                operations = {
                    "+": lambda: self.stringify(left) + self.stringify(right) if isinstance(left, str) or isinstance(right, str) else left + right,
                    "-": lambda: left - right, "*": lambda: left * right,
                    "/": lambda: left / right, "==": lambda: left == right,
                    "!=": lambda: left != right, "<": lambda: left < right,
                    ">": lambda: left > right, "<=": lambda: left <= right,
                    ">=": lambda: left >= right,
                }
                if node.op == "/" and right == 0: raise MintRuntimeError("Division by zero")
                return operations[node.op]()
            except MintRuntimeError: raise
            except (TypeError, ValueError, KeyError) as exc: raise MintRuntimeError(f"Invalid operation '{node.op}': {exc}") from exc
        if isinstance(node, Let):
            value = self.eval(node.expr); self.env.define(node.name, value); return value
        if isinstance(node, Assign):
            value = self.eval(node.expr); self.env.assign(node.name, value); return value
        if isinstance(node, Print):
            value = self.eval(node.expr); print(self.stringify(value)); return value
        if isinstance(node, If):
            if self.is_truthy(self.eval(node.condition)): self.execute_block(node.then_branch, Environment(self.env))
            elif node.else_branch is not None: self.execute_block(node.else_branch, Environment(self.env))
            return None
        if isinstance(node, While):
            while self.is_truthy(self.eval(node.condition)):
                try: self.execute_block(node.body, Environment(self.env))
                except BreakException: break
                except ContinueException: continue
            return None
        if isinstance(node, For):
            iterable = self.eval(node.iterable)
            if not isinstance(iterable, (list, dict, str)):
                raise MintRuntimeError(f"'for' expects a list, dictionary or string, got {self.type_name(iterable)}")
            items = iterable.keys() if isinstance(iterable, dict) else iterable
            for item in items:
                local = Environment(self.env); local.define(node.var_name, item)
                try: self.execute_block(node.body, local)
                except BreakException: break
                except ContinueException: continue
            return None
        if isinstance(node, Block): self.execute_block(node.statements, Environment(self.env)); return None
        if isinstance(node, FunctionDef):
            fn = MintFunction(node, self.env); self.env.define(node.name, fn); return fn
        if isinstance(node, FunctionCall):
            callee = self.eval(node.callee); args = [self.eval(arg) for arg in node.arguments]
            if isinstance(callee, tuple) and callee[0] == "builtin": return callee[1](args)
            if isinstance(callee, MintClass):
                instance = MintInstance(callee)
                initializer = callee.find_method("init")
                if initializer is not None: self.call_function(initializer, args, instance)
                elif args: raise MintRuntimeError(f"Class '{callee.name}' takes no constructor arguments")
                return instance
            return self.call_function(callee, args)
        if isinstance(node, Return): raise ReturnValue(self.eval(node.value) if node.value is not None else None)
        if isinstance(node, ListLiteral): return [self.eval(item) for item in node.elements]
        if isinstance(node, DictLiteral):
            result = {}
            for key_node, value_node in node.pairs:
                key = self.eval(key_node)
                if not isinstance(key, (str, int, float, bool)): raise MintRuntimeError("Dictionary keys must be strings, numbers or booleans")
                result[key] = self.eval(value_node)
            return result
        if isinstance(node, Index):
            obj, idx = self.eval(node.obj), self.eval(node.index)
            try:
                if isinstance(obj, (list, str)):
                    if not isinstance(idx, int): raise MintRuntimeError("Sequence index must be an integer")
                    return obj[idx]
                if isinstance(obj, dict): return obj[idx]
            except IndexError: raise MintRuntimeError(f"Index {idx} out of range")
            except KeyError: raise MintRuntimeError(f"Key {idx!r} not found")
            raise MintRuntimeError(f"Cannot index type {self.type_name(obj)}")
        if isinstance(node, SetIndex):
            obj, idx, value = self.eval(node.obj), self.eval(node.index), self.eval(node.value)
            if isinstance(obj, list):
                if not isinstance(idx, int): raise MintRuntimeError("List index must be an integer")
                try: obj[idx] = value
                except IndexError: raise MintRuntimeError(f"List index {idx} out of range")
                return value
            if isinstance(obj, dict):
                if not isinstance(idx, (str, int, float, bool)):
                    raise MintRuntimeError("Dictionary keys must be strings, numbers or booleans")
                obj[idx] = value
                return value
            raise MintRuntimeError(f"Cannot assign an index on type {self.type_name(obj)}")
        if isinstance(node, GetAttr): return self.get_attribute(self.eval(node.obj), node.name)
        if isinstance(node, SetAttr):
            obj, value = self.eval(node.obj), self.eval(node.value)
            if not isinstance(obj, MintInstance): raise MintRuntimeError("Only instances have assignable attributes")
            obj.fields[node.name] = value; return value
        if isinstance(node, ClassDef):
            methods_env = Environment(self.env)
            methods = {method.name: MintFunction(method, methods_env) for method in node.methods}
            klass = MintClass(node.name, methods); self.env.define(node.name, klass); return klass
        if isinstance(node, Import):
            module = self.import_module(node.path); self.env.define(node.alias, module); return module
        if isinstance(node, Break): raise BreakException()
        if isinstance(node, Continue): raise ContinueException()
        raise MintRuntimeError(f"Unknown syntax node: {type(node).__name__}")

    def execute_block(self, statements, env):
        previous = self.env
        try:
            self.env = env
            for statement in statements: self.eval(statement)
        finally: self.env = previous

    def run(self, statements, source_path=None):
        if source_path is not None: self.source_dir = Path(source_path).resolve().parent
        for statement in statements: self.eval(statement)
