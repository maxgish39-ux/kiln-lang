"""Recursive-descent parser for Kiln."""
from .ast import (
    Number, String, Boolean, Var, BinOp, UnaryOp, Let, Assign, Print,
    If, While, For, Block, FunctionDef, FunctionCall, Return,
    ListLiteral, DictLiteral, Index, Break, Continue, ClassDef,
    GetAttr, SetAttr, SetIndex, Import,
)


class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def current(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else (None, None, None, None)

    def peek(self, offset=1):
        idx = self.pos + offset
        return self.tokens[idx] if idx < len(self.tokens) else (None, None, None, None)

    def eat(self, kind):
        tok = self.current()
        if tok[0] != kind:
            line = tok[2] if tok[2] is not None else '?'
            col = tok[3] if tok[3] is not None else '?'
            raise SyntaxError(f"Expected {kind}, got {tok[0]} at line {line}, column {col}")
        self.pos += 1
        return tok[1]

    def optional_semicolon(self):
        if self.current()[0] == 'SEMI':
            self.eat('SEMI')

    def parse(self):
        statements = []
        while self.current()[0] is not None:
            before = self.pos
            statements.append(self.statement())
            if self.pos == before:
                raise SyntaxError("Parser made no progress")
        return statements

    def statement(self):
        kind = self.current()[0]
        if kind == 'LET': return self.let_stmt()
        if kind == 'FUN': return self.function_def()
        if kind == 'CLASS': return self.class_def()
        if kind == 'IMPORT': return self.import_stmt()
        if kind == 'RETURN': return self.return_stmt()
        if kind == 'PRINT': return self.print_stmt()
        if kind == 'IF': return self.if_stmt()
        if kind == 'WHILE': return self.while_stmt()
        if kind == 'FOR': return self.for_stmt()
        if kind == 'BREAK':
            self.eat('BREAK'); self.optional_semicolon(); return Break()
        if kind == 'CONTINUE':
            self.eat('CONTINUE'); self.optional_semicolon(); return Continue()
        if kind == 'LBRACE': return self.block()
        expr = self.expr()
        if self.current()[0] == 'EQ':
            self.eat('EQ')
            value = self.expr()
            self.optional_semicolon()
            if isinstance(expr, Var): return Assign(expr.name, value)
            if isinstance(expr, GetAttr): return SetAttr(expr.obj, expr.name, value)
            if isinstance(expr, Index): return SetIndex(expr.obj, expr.index, value)
            tok = self.current()
            raise SyntaxError("Invalid assignment target")
        self.optional_semicolon()
        return expr

    def let_stmt(self):
        self.eat('LET'); name = self.eat('ID'); self.eat('EQ')
        expr = self.expr(); self.optional_semicolon()
        return Let(name, expr)

    def import_stmt(self):
        self.eat('IMPORT')
        path = self.eat('STRING')
        self.eat('AS')
        alias = self.eat('ID')
        self.optional_semicolon()
        return Import(path, alias)

    def function_def(self):
        self.eat('FUN'); name = self.eat('ID'); self.eat('LPAREN')
        params = self.parameters(); self.eat('RPAREN')
        return FunctionDef(name, params, self.block().statements)

    def class_def(self):
        self.eat('CLASS'); name = self.eat('ID'); self.eat('LBRACE')
        methods = []
        while self.current()[0] not in ('RBRACE', None):
            if self.current()[0] != 'FUN':
                tok = self.current()
                raise SyntaxError(f"Expected method declaration in class at line {tok[2]}, column {tok[3]}")
            methods.append(self.function_def())
        self.eat('RBRACE')
        return ClassDef(name, methods)

    def parameters(self):
        params = []
        if self.current()[0] != 'RPAREN':
            params.append(self.eat('ID'))
            while self.current()[0] == 'COMMA':
                self.eat('COMMA'); params.append(self.eat('ID'))
        if len(params) != len(set(params)):
            raise SyntaxError("Duplicate function parameter")
        return params

    def return_stmt(self):
        self.eat('RETURN'); value = None
        if self.current()[0] not in ('SEMI', 'RBRACE', None): value = self.expr()
        self.optional_semicolon()
        return Return(value)

    def print_stmt(self):
        self.eat('PRINT'); self.eat('LPAREN'); expr = self.expr(); self.eat('RPAREN')
        self.optional_semicolon(); return Print(expr)

    def if_stmt(self):
        self.eat('IF'); condition = self.expr(); then = self.block(); otherwise = None
        if self.current()[0] == 'ELSE':
            self.eat('ELSE'); otherwise = self.block()
        return If(condition, then.statements, otherwise.statements if otherwise else None)

    def while_stmt(self):
        self.eat('WHILE'); condition = self.expr(); return While(condition, self.block().statements)

    def for_stmt(self):
        self.eat('FOR'); name = self.eat('ID'); self.eat('IN'); iterable = self.expr()
        return For(name, iterable, self.block().statements)

    def block(self):
        self.eat('LBRACE'); statements = []
        while self.current()[0] not in ('RBRACE', None):
            statements.append(self.statement())
        self.eat('RBRACE')
        return Block(statements)

    def expr(self): return self.logic_or()
    def logic_or(self):
        node = self.logic_and()
        while self.current()[0] == 'OR':
            self.eat('OR'); node = BinOp(node, 'or', self.logic_and())
        return node
    def logic_and(self):
        node = self.equality()
        while self.current()[0] == 'AND':
            self.eat('AND'); node = BinOp(node, 'and', self.equality())
        return node
    def equality(self):
        node = self.comparison()
        while self.current()[0] in ('EQEQ', 'NEQ'):
            op = self.eat(self.current()[0]); node = BinOp(node, op, self.comparison())
        return node
    def comparison(self):
        node = self.term()
        while self.current()[0] in ('LT', 'GT', 'LTE', 'GTE'):
            op = self.eat(self.current()[0]); node = BinOp(node, op, self.term())
        return node
    def term(self):
        node = self.factor()
        while self.current()[0] in ('PLUS', 'MINUS'):
            op = self.eat(self.current()[0]); node = BinOp(node, op, self.factor())
        return node
    def factor(self):
        node = self.unary()
        while self.current()[0] in ('MUL', 'DIV'):
            op = self.eat(self.current()[0]); node = BinOp(node, op, self.unary())
        return node
    def unary(self):
        if self.current()[0] in ('NOT', 'MINUS', 'PLUS'):
            op = self.eat(self.current()[0]); return UnaryOp(op, self.unary())
        return self.call()
    def call(self):
        node = self.primary()
        while True:
            if self.current()[0] == 'LPAREN':
                self.eat('LPAREN'); args = []
                if self.current()[0] != 'RPAREN':
                    args.append(self.expr())
                    while self.current()[0] == 'COMMA':
                        self.eat('COMMA'); args.append(self.expr())
                self.eat('RPAREN'); node = FunctionCall(node, args)
            elif self.current()[0] == 'LBRACK':
                self.eat('LBRACK'); idx = self.expr(); self.eat('RBRACK'); node = Index(node, idx)
            elif self.current()[0] == 'DOT':
                self.eat('DOT'); node = GetAttr(node, self.eat('ID'))
            else: break
        return node

    def primary(self):
        kind, value, line, col = self.current()
        if kind == 'NUMBER': self.eat(kind); return Number(value)
        if kind == 'STRING': self.eat(kind); return String(value)
        if kind in ('TRUE', 'FALSE'): self.eat(kind); return Boolean(value)
        if kind == 'ID': self.eat(kind); return Var(value)
        if kind == 'LBRACK': return self.list_literal()
        if kind == 'LBRACE': return self.dict_literal()
        if kind == 'LPAREN':
            self.eat('LPAREN'); node = self.expr(); self.eat('RPAREN'); return node
        raise SyntaxError(f"Unexpected token {kind} at line {line}, column {col}")

    def list_literal(self):
        self.eat('LBRACK'); items = []
        if self.current()[0] != 'RBRACK':
            items.append(self.expr())
            while self.current()[0] == 'COMMA': self.eat('COMMA'); items.append(self.expr())
        self.eat('RBRACK'); return ListLiteral(items)

    def dict_literal(self):
        self.eat('LBRACE'); pairs = []
        if self.current()[0] != 'RBRACE':
            pairs.append(self.dict_pair())
            while self.current()[0] == 'COMMA': self.eat('COMMA'); pairs.append(self.dict_pair())
        self.eat('RBRACE'); return DictLiteral(pairs)

    def dict_pair(self):
        key = self.expr(); self.eat('COLON'); return key, self.expr()
