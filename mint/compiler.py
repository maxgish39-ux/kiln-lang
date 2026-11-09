"""Native C backend for the portable MINT language subset."""
import json
import re

from .ast import (
    Number, String, Boolean, Var, BinOp, UnaryOp, Let, Assign, Print,
    If, While, Block, FunctionDef, FunctionCall, Return,
)


class NativeCompileError(Exception):
    pass


class CCompiler:
    def __init__(self):
        self.globals = set()
        self.functions = {}
        self.locals = set()
        self.params = set()
        self.in_function = False

    @staticmethod
    def _valid_name(name):
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            raise NativeCompileError(f"Unsupported identifier: {name!r}")

    def compile(self, statements):
        self.globals, self.functions = set(), {}
        for node in statements:
            if isinstance(node, FunctionDef):
                self._valid_name(node.name)
                if node.name in self.functions:
                    raise NativeCompileError(f"Function '{node.name}' is defined more than once")
                self.functions[node.name] = node
        for node in statements:
            if not isinstance(node, FunctionDef):
                self._collect_globals([node])

        for fn in self.functions.values():
            for param in fn.params:
                self._valid_name(param)

        prototypes = "\n".join(
            f"static Value mint_fn_{name}(int argc, Value *argv);"
            for name in self.functions
        )
        global_decls = "\n".join(
            f"static Value mint_g_{name} = {{V_NUMBER, 0, NULL, 0}};"
            for name in sorted(self.globals)
        )
        functions = "\n\n".join(self._compile_function(fn) for fn in self.functions.values())
        main_body = "\n".join(
            self.statement(node, 1) for node in statements if not isinstance(node, FunctionDef)
        )
        return (
            RUNTIME_C.replace("/* MINT_GLOBALS */", global_decls)
            .replace("/* MINT_PROTOTYPES */", prototypes)
            .replace("/* MINT_FUNCTIONS */", functions)
            .replace("/* MINT_PROGRAM */", main_body)
        )

    def _collect_globals(self, nodes):
        for node in nodes:
            if isinstance(node, Let):
                self._valid_name(node.name)
                self.globals.add(node.name)
            elif isinstance(node, Assign):
                self._valid_name(node.name)
            elif isinstance(node, If):
                self._collect_globals(node.then_branch)
                if node.else_branch:
                    self._collect_globals(node.else_branch)
            elif isinstance(node, While):
                self._collect_globals(node.body)
            elif isinstance(node, Block):
                self._collect_globals(node.statements)
            elif isinstance(node, (Print, FunctionCall)):
                if isinstance(node, FunctionCall):
                    self.expression(node)
            else:
                raise NativeCompileError(f"Native compiler does not support {type(node).__name__}")

    def _collect_locals(self, nodes):
        for node in nodes:
            if isinstance(node, Let):
                self._valid_name(node.name)
                self.locals.add(node.name)
            elif isinstance(node, (Assign, Print, Return)):
                pass
            elif isinstance(node, If):
                self._collect_locals(node.then_branch)
                if node.else_branch:
                    self._collect_locals(node.else_branch)
            elif isinstance(node, While):
                self._collect_locals(node.body)
            elif isinstance(node, Block):
                self._collect_locals(node.statements)
            else:
                raise NativeCompileError(
                    f"Native compiler does not support function statement {type(node).__name__}"
                )

    def _compile_function(self, fn):
        self.in_function = True
        self.locals, self.params = set(), set(fn.params)
        self._collect_locals(fn.body)
        declarations = [
            f"    Value mint_l_{param} = argv[{index}];"
            for index, param in enumerate(fn.params)
        ]
        declarations.extend(
            f"    Value mint_l_{name} = number(0);"
            for name in sorted(self.locals - self.params)
        )
        body = "\n".join(self.statement(node, 1) for node in fn.body)
        self.in_function, self.locals, self.params = False, set(), set()
        header = (
            f'static Value mint_fn_{fn.name}(int argc, Value *argv) {{\n'
            f'    if (argc != {len(fn.params)}) '
            f'mint_arity_error("{fn.name}", {len(fn.params)}, argc);\n'
        )
        return (
            header + "\n".join(declarations) + ("\n" if declarations else "")
            + body + "\n    return boolean_value(0);\n}"
        )

    def _resolve(self, name):
        self._valid_name(name)
        if self.in_function and name in self.locals | self.params:
            return "mint_l_" + name
        if name in self.globals:
            return "mint_g_" + name
        raise NativeCompileError(f"Undefined variable: {name}")

    def expression(self, node):
        if isinstance(node, Number):
            return f"number({float(node.value)!r})"
        if isinstance(node, String):
            return f"string_value({json.dumps(node.value, ensure_ascii=True)})"
        if isinstance(node, Boolean):
            return f"boolean_value({int(node.value)})"
        if isinstance(node, Var):
            if node.name in self.functions:
                raise NativeCompileError(f"Function '{node.name}' must be called")
            return self._resolve(node.name)
        if isinstance(node, UnaryOp):
            value = self.expression(node.operand)
            if node.op == "-":
                return f"number(-as_number({value}))"
            if node.op == "+":
                return f"number(as_number({value}))"
            if node.op == "not":
                return f"boolean_value(!truthy({value}))"
        if isinstance(node, BinOp):
            left, right = self.expression(node.left), self.expression(node.right)
            if node.op in ("+", "-", "*", "/"):
                return f"binary_value({json.dumps(node.op)}, {left}, {right})"
            if node.op in ("==", "!=", "<", ">", "<=", ">="):
                return f"compare_value({json.dumps(node.op)}, {left}, {right})"
            if node.op == "and":
                return f"boolean_value(truthy({left}) && truthy({right}))"
            if node.op == "or":
                return f"boolean_value(truthy({left}) || truthy({right}))"
        if isinstance(node, FunctionCall):
            if not isinstance(node.callee, Var) or node.callee.name not in self.functions:
                raise NativeCompileError("Native calls must target a declared top-level function")
            name = node.callee.name
            expected = len(self.functions[name].params)
            if len(node.arguments) != expected:
                raise NativeCompileError(
                    f"Function '{name}' expects {expected} argument(s), got {len(node.arguments)}"
                )
            args = [self.expression(arg) for arg in node.arguments]
            array = "(Value[]){" + ", ".join(args) + "}" if args else "NULL"
            return f"mint_fn_{name}({len(args)}, {array})"
        raise NativeCompileError(f"Native compiler cannot compile expression {type(node).__name__}")

    def statement(self, node, depth):
        pad = "    " * depth
        if isinstance(node, (Let, Assign)):
            return f"{pad}{self._resolve(node.name)} = {self.expression(node.expr)};"
        if isinstance(node, Print):
            return f"{pad}print_value({self.expression(node.expr)});"
        if isinstance(node, FunctionCall):
            return f"{pad}{self.expression(node)};"
        if isinstance(node, Return):
            if not self.in_function:
                raise NativeCompileError("'return' is only supported inside functions")
            value = "boolean_value(0)" if node.value is None else self.expression(node.value)
            return f"{pad}return {value};"
        if isinstance(node, If):
            out = (
                f"{pad}if (truthy({self.expression(node.condition)})) {{\n"
                + "\n".join(self.statement(item, depth + 1) for item in node.then_branch)
                + f"\n{pad}}}"
            )
            if node.else_branch is not None:
                out += (
                    " else {\n"
                    + "\n".join(self.statement(item, depth + 1) for item in node.else_branch)
                    + f"\n{pad}}}"
                )
            return out
        if isinstance(node, While):
            return (
                f"{pad}while (truthy({self.expression(node.condition)})) {{\n"
                + "\n".join(self.statement(item, depth + 1) for item in node.body)
                + f"\n{pad}}}"
            )
        if isinstance(node, Block):
            return (
                pad + "{\n"
                + "\n".join(self.statement(item, depth + 1) for item in node.statements)
                + f"\n{pad}}}"
            )
        raise NativeCompileError(f"Native compiler does not support statement {type(node).__name__}")


RUNTIME_C = r'''#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef enum { V_NUMBER, V_STRING, V_BOOL } ValueType;
typedef struct { ValueType type; double n; const char *s; int b; } Value;
static Value number(double n){Value v={V_NUMBER,n,NULL,0};return v;}
static Value string_value(const char*s){Value v={V_STRING,0,s,0};return v;}
static Value boolean_value(int b){Value v={V_BOOL,0,NULL,!!b};return v;}
static double as_number(Value v){if(v.type==V_NUMBER)return v.n;if(v.type==V_BOOL)return v.b?1:0;fprintf(stderr,"MINT runtime: expected number\n");exit(2);}
static int truthy(Value v){if(v.type==V_BOOL)return v.b;if(v.type==V_NUMBER)return v.n!=0;return v.s&&v.s[0];}
static const char*as_string(Value v){if(v.type==V_STRING)return v.s?v.s:"";static char buf[8][64];static int i=0;char*p=buf[(i++)%8];if(v.type==V_BOOL)snprintf(p,64,"%s",v.b?"true":"false");else snprintf(p,64,"%.15g",v.n);return p;}
static void mint_arity_error(const char*n,int e,int a){fprintf(stderr,"MINT runtime: %s expects %d argument(s), got %d\n",n,e,a);exit(2);}
static Value binary_value(const char*op,Value a,Value b){if(!strcmp(op,"+")&&(a.type==V_STRING||b.type==V_STRING)){const char*x=as_string(a),*y=as_string(b);size_t n=strlen(x)+strlen(y)+1;char*p=malloc(n);if(!p)exit(2);snprintf(p,n,"%s%s",x,y);return string_value(p);}double x=as_number(a),y=as_number(b);if(!strcmp(op,"+"))return number(x+y);if(!strcmp(op,"-"))return number(x-y);if(!strcmp(op,"*"))return number(x*y);if(!strcmp(op,"/")){if(y==0){fprintf(stderr,"MINT runtime: division by zero\n");exit(2);}return number(x/y);}return number(0);}
static Value compare_value(const char*op,Value a,Value b){int r=0;if(a.type==V_STRING&&b.type==V_STRING){int c=strcmp(a.s?a.s:"",b.s?b.s:"");if(!strcmp(op,"=="))r=c==0;else if(!strcmp(op,"!="))r=c!=0;else if(!strcmp(op,"<"))r=c<0;else if(!strcmp(op,">"))r=c>0;else if(!strcmp(op,"<="))r=c<=0;else if(!strcmp(op,">="))r=c>=0;}else{double x=as_number(a),y=as_number(b);if(!strcmp(op,"=="))r=x==y;else if(!strcmp(op,"!="))r=x!=y;else if(!strcmp(op,"<"))r=x<y;else if(!strcmp(op,">"))r=x>y;else if(!strcmp(op,"<="))r=x<=y;else if(!strcmp(op,">="))r=x>=y;}return boolean_value(r);}
static void print_value(Value v){if(v.type==V_STRING)puts(v.s?v.s:"");else if(v.type==V_BOOL)puts(v.b?"true":"false");else printf("%.15g\n",v.n);}
/* MINT_GLOBALS */
/* MINT_PROTOTYPES */
/* MINT_FUNCTIONS */
int main(void){
/* MINT_PROGRAM */
return 0;
}
'''
