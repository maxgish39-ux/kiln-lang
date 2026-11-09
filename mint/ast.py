"""Abstract syntax tree nodes for MINT."""

class Number:
    def __init__(self, value): self.value = value
class String:
    def __init__(self, value): self.value = value
class Boolean:
    def __init__(self, value): self.value = value
class Var:
    def __init__(self, name): self.name = name
class BinOp:
    def __init__(self, left, op, right): self.left, self.op, self.right = left, op, right
class UnaryOp:
    def __init__(self, op, operand): self.op, self.operand = op, operand
class Let:
    def __init__(self, name, expr): self.name, self.expr = name, expr
class Assign:
    def __init__(self, name, expr): self.name, self.expr = name, expr
class Print:
    def __init__(self, expr): self.expr = expr
class If:
    def __init__(self, condition, then_branch, else_branch=None): self.condition, self.then_branch, self.else_branch = condition, then_branch, else_branch
class While:
    def __init__(self, condition, body): self.condition, self.body = condition, body
class For:
    def __init__(self, var_name, iterable, body): self.var_name, self.iterable, self.body = var_name, iterable, body
class Block:
    def __init__(self, statements): self.statements = statements
class FunctionDef:
    def __init__(self, name, params, body): self.name, self.params, self.body = name, params, body
class FunctionCall:
    def __init__(self, callee, arguments): self.callee, self.arguments = callee, arguments
class Return:
    def __init__(self, value=None): self.value = value
class ListLiteral:
    def __init__(self, elements): self.elements = elements
class DictLiteral:
    def __init__(self, pairs): self.pairs = pairs
class Index:
    def __init__(self, obj, index): self.obj, self.index = obj, index
class Break: pass
class Continue: pass
class ClassDef:
    def __init__(self, name, methods): self.name, self.methods = name, methods
class GetAttr:
    def __init__(self, obj, name): self.obj, self.name = obj, name
class SetAttr:
    def __init__(self, obj, name, value): self.obj, self.name, self.value = obj, name, value
class SetIndex:
    def __init__(self, obj, index, value): self.obj, self.index, self.value = obj, index, value
class Import:
    def __init__(self, path, alias): self.path, self.alias = path, alias
