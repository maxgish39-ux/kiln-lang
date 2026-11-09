# mint/lexer.py
import re

TOKEN_SPEC = [
    ('NUMBER',   r'\d+(\.\d*)?'),
    ('STRING',   r'"[^"]*"'),
    ('CLASS',    r'\bclass\b'),
    ('IMPORT',   r'\bimport\b'),
    ('AS',       r'\bas\b'),
    ('LET',      r'\blet\b'),
    ('FUN',      r'\bfun\b'),
    ('RETURN',   r'\breturn\b'),
    ('PRINT',    r'\bprint\b'),
    ('IF',       r'\bif\b'),
    ('ELSE',     r'\belse\b'),
    ('WHILE',    r'\bwhile\b'),
    ('FOR',      r'\bfor\b'),
    ('IN',       r'\bin\b'),
    ('BREAK',    r'\bbreak\b'),
    ('CONTINUE', r'\bcontinue\b'),
    ('TRUE',     r'\btrue\b'),
    ('FALSE',    r'\bfalse\b'),
    ('AND',      r'\band\b'),
    ('OR',       r'\bor\b'),
    ('NOT',      r'\bnot\b'),
    ('ID',       r'[a-zA-Z_][a-zA-Z0-9_]*'),
    ('EQEQ',     r'=='),
    ('NEQ',      r'!='),
    ('LTE',      r'<='),
    ('GTE',      r'>='),
    ('LT',       r'<'),
    ('GT',       r'>'),
    ('DOT',      r'\.'),
    ('EQ',       r'='),
    ('PLUS',     r'\+'),
    ('MINUS',    r'-'),
    ('MUL',      r'\*'),
    ('DIV',      r'/'),
    ('LPAREN',   r'\('),
    ('RPAREN',   r'\)'),
    ('LBRACE',   r'\{'),
    ('RBRACE',   r'\}'),
    ('LBRACK',   r'\['),
    ('RBRACK',   r'\]'),
    ('COLON',    r':'),
    ('SEMI',     r';'),
    ('COMMA',    r','),
    ('SKIP',     r'[ \t\r\n]+'),
    ('COMMENT',  r'#.*'),
    ('MISMATCH', r'.'),
]

_tok_regex = '|'.join(f'(?P<{name}>{pattern})' for name, pattern in TOKEN_SPEC)
_compiled = re.compile(_tok_regex)

def tokenize(code: str):
    tokens = []
    line = 1
    line_start = 0

    for mo in _compiled.finditer(code):
        kind = mo.lastgroup
        value = mo.group()
        start = mo.start()

        while line_start < start:
            if code[line_start] == '\n':
                line += 1
                line_start += 1
            else:
                break
        column = start - line_start + 1

        if kind == 'NUMBER':
            value = float(value) if '.' in value else int(value)
        elif kind == 'STRING':
            value = value[1:-1]
        elif kind == 'TRUE':
            value = True
        elif kind == 'FALSE':
            value = False
        elif kind in ('SKIP', 'COMMENT'):
            for i, ch in enumerate(value):
                if ch == '\n':
                    line += 1
                    line_start = start + i + 1
            continue
        elif kind == 'MISMATCH':
            raise SyntaxError(f'Unexpected character {value!r} at line {line}, column {column}')

        tokens.append((kind, value, line, column))

    return tokens
