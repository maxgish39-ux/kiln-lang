/* Bytecode compiler and virtual machine (the default executor).
 *
 * The compiler turns a statement list into a Chunk of instructions. It handles
 * literals, names, operators, plain calls, if, while, for (single scope), break,
 * continue and return. Anything else becomes I_EVAL, which runs that node with
 * the tree-walker in eval(). Scopes stay name-based Env records, so compiled and
 * interpreted code share one set of semantics and can be mixed freely.
 * `kiln --tree` skips the compiler and runs everything through eval().
 */
#include <alloca.h>

typedef enum {
    I_CONST, I_GET, I_DEFINE, I_SETVAR, I_POP, I_STORELAST, I_BIN, I_UN,
    I_JMP, I_JF, I_ANDJ, I_ORJ, I_CALL, I_EVAL, I_SIG, I_BREAK, I_CONT, I_RET,
    I_LOOP_BEGIN, I_LOOP_END, I_FOR_PREP, I_FOR_NEXT, I_FOR_END, I_END
} Op;

static const char *op_names[] = {
    "CONST", "GET", "DEFINE", "SETVAR", "POP", "STORELAST", "BIN", "UN",
    "JMP", "JF", "ANDJ", "ORJ", "CALL", "EVAL", "SIG", "BREAK", "CONT", "RET",
    "LOOP_BEGIN", "LOOP_END", "FOR_PREP", "FOR_NEXT", "FOR_END", "END"
};

typedef struct Chunk Chunk;
typedef struct { uint8_t op; int a; int line; void *p; } Instr;
typedef struct { int brk, cont; } LoopRec;
struct Chunk {
    Instr *code; int n, cap;
    Value *consts; int nconst, capconst;
    LoopRec *loops; int nloops, capl;
    int maxdepth;
};

typedef struct { Chunk *c; int depth, line, loop; } Compiler;

static bool g_use_vm = true;

static int emit(Compiler *k, int op, int a, void *p, int delta) {
    Chunk *c = k->c;
    if (c->n == c->cap) {
        c->cap = c->cap ? c->cap * 2 : 32;
        c->code = realloc(c->code, (size_t)c->cap * sizeof(Instr));
    }
    Instr *in = &c->code[c->n];
    in->op = (uint8_t)op; in->a = a; in->line = k->line; in->p = p;
    k->depth += delta;
    if (k->depth > c->maxdepth) c->maxdepth = k->depth;
    return c->n++;
}

static int add_const(Compiler *k, Value v) {
    Chunk *c = k->c;
    if (c->nconst == c->capconst) {
        c->capconst = c->capconst ? c->capconst * 2 : 16;
        c->consts = realloc(c->consts, (size_t)c->capconst * sizeof(Value));
    }
    c->consts[c->nconst] = v;
    return c->nconst++;
}

static int new_loop(Compiler *k) {
    Chunk *c = k->c;
    if (c->nloops == c->capl) {
        c->capl = c->capl ? c->capl * 2 : 8;
        c->loops = realloc(c->loops, (size_t)c->capl * sizeof(LoopRec));
    }
    c->loops[c->nloops] = (LoopRec){0, 0};
    return c->nloops++;
}

/* True when a block defines names directly; such blocks need their own scope. */
static bool defines_names(Node *blk) {
    for (Node *x = blk ? blk->next : NULL; x; x = x->next)
        if (x->t == N_LET || x->t == N_FUN || x->t == N_CLASS || x->t == N_IMPORT) return true;
    return false;
}

/* Same rule as the tree-walker's for-loop: 1 reuses one scope, 2 needs a scope per pass. */
static int for_mode(Node *n) {
    if (!n->opc) {
        n->opc = contains_closure(n->b) ? 2 : 1;
        if (defines_names(n->b)) n->opc = 2;
    }
    return n->opc;
}

/* Expression nodes the compiler emits directly (others become I_EVAL). */
static bool compilable_expr(Node *n) {
    switch (n->t) {
    case N_NUM: case N_STR: case N_BOOL: case N_NULL: case N_VAR:
    case N_BIN: case N_UN: case N_LET: case N_SET:
        return true;
    case N_CALL:
        return n->a->t == N_VAR && n->opc != 1;
    default:
        return false;
    }
}

static void compile_stmt(Compiler *k, Node *n);

static void compile_expr(Compiler *k, Node *n) {
    if (!n) { emit(k, I_CONST, add_const(k, nul()), NULL, 1); return; }
    if (n->line) k->line = n->line;
    switch (n->t) {
    case N_NUM: case N_STR: case N_BOOL:
        emit(k, I_CONST, add_const(k, n->val), NULL, 1);
        return;
    case N_NULL:
        emit(k, I_CONST, add_const(k, nul()), NULL, 1);
        return;
    case N_VAR:
        emit(k, I_GET, 0, n->name, 1);
        return;
    case N_BIN:
        if (n->opc == OP_AND || n->opc == OP_OR) {
            compile_expr(k, n->a);
            int j = emit(k, n->opc == OP_AND ? I_ANDJ : I_ORJ, 0, NULL, -1);
            compile_expr(k, n->b);
            k->c->code[j].a = k->c->n;
            return;
        }
        compile_expr(k, n->a);
        compile_expr(k, n->b);
        emit(k, I_BIN, 0, n, -1);
        return;
    case N_UN:
        compile_expr(k, n->a);
        emit(k, I_UN, 0, n, 0);
        return;
    case N_CALL:
        if (n->a->t == N_VAR && n->opc != 1) {
            for (int i = 0; i < n->argc; i++) compile_expr(k, n->args[i]);
            emit(k, I_CALL, n->argc, n->a->name, 1 - n->argc);
            return;
        }
        break;
    case N_LET:
        compile_expr(k, n->a);
        emit(k, I_DEFINE, 0, n->name, 0);
        return;
    case N_SET:
        compile_expr(k, n->a);
        emit(k, I_SETVAR, 0, n->name, 0);
        return;
    default:
        break;
    }
    emit(k, I_EVAL, 0, n, 1);
}

/* Statements in nested blocks: each leaves one value, which is discarded. */
static void compile_nested(Compiler *k, Node *first) {
    for (Node *x = first; x; x = x->next) {
        compile_stmt(k, x);
        emit(k, I_POP, 0, NULL, -1);
    }
}

static void compile_if(Compiler *k, Node *n) {
    compile_expr(k, n->a);
    int jf = emit(k, I_JF, 0, NULL, -1);
    compile_nested(k, n->b->next);
    int jend = emit(k, I_JMP, 0, NULL, 0);
    k->c->code[jf].a = k->c->n;
    if (n->c) compile_nested(k, n->c->next);
    k->c->code[jend].a = k->c->n;
    emit(k, I_CONST, add_const(k, nul()), NULL, 1);
}

static void compile_while(Compiler *k, Node *n) {
    int saved_loop = k->loop;
    int li = new_loop(k);
    emit(k, I_LOOP_BEGIN, 0, NULL, 0);
    int top = k->c->n;
    k->loop = li;
    compile_expr(k, n->a);
    int jf = emit(k, I_JF, 0, NULL, -1);
    compile_nested(k, n->b->next);
    emit(k, I_JMP, top, NULL, 0);
    int exit_at = k->c->n;
    k->c->code[jf].a = exit_at;
    k->loop = saved_loop;
    k->c->loops[li].brk = exit_at;
    k->c->loops[li].cont = top;
    emit(k, I_LOOP_END, 0, NULL, 0);
    emit(k, I_CONST, add_const(k, nul()), NULL, 1);
}

static void compile_for(Compiler *k, Node *n) {
    int saved_loop = k->loop;
    compile_expr(k, n->a);
    emit(k, I_FOR_PREP, 0, n->name, 3);
    int li = new_loop(k);
    k->loop = li;
    int head = emit(k, I_FOR_NEXT, 0, n->name, 0);
    compile_nested(k, n->b->next);
    emit(k, I_JMP, head, NULL, 0);
    int exit_at = k->c->n;
    k->c->code[head].a = exit_at;
    k->loop = saved_loop;
    k->c->loops[li].brk = exit_at;
    k->c->loops[li].cont = head;
    emit(k, I_FOR_END, 0, NULL, -4);
    emit(k, I_CONST, add_const(k, nul()), NULL, 1);
}

static void compile_stmt(Compiler *k, Node *n) {
    if (n->line) k->line = n->line;
    switch (n->t) {
    case N_IF:
        if (defines_names(n->b) || (n->c && defines_names(n->c))) break;
        compile_if(k, n);
        return;
    case N_WHILE:
        if (defines_names(n->b)) break;
        compile_while(k, n);
        return;
    case N_FOR:
        if (for_mode(n) != 1) break;
        compile_for(k, n);
        return;
    case N_BLOCK:
        if (defines_names(n)) break;
        compile_nested(k, n->next);
        emit(k, I_CONST, add_const(k, nul()), NULL, 1);
        return;
    case N_BREAK:
    case N_CONTINUE:
        emit(k, n->t == N_BREAK ? I_BREAK : I_CONT, k->loop, NULL, 0);
        emit(k, I_CONST, add_const(k, nul()), NULL, 1);  /* unreachable; keeps the stack count */
        return;
    case N_RET:
        compile_expr(k, n->a);
        emit(k, I_RET, 0, NULL, -1);
        emit(k, I_CONST, add_const(k, nul()), NULL, 1);  /* unreachable */
        return;
    default:
        break;
    }
    if (compilable_expr(n)) {
        compile_expr(k, n);
        return;
    }
    /* Fallback: the tree-walker runs the statement; check for break/continue/return it raised. */
    emit(k, I_EVAL, 0, n, 1);
    emit(k, I_SIG, k->loop, NULL, 0);
}

static Chunk *compile_body(Node *blk) {
    Compiler k = {0};
    k.c = calloc(1, sizeof(Chunk));
    k.loop = -1;
    k.line = blk->line;
    for (Node *x = blk->next; x; x = x->next) {
        compile_stmt(&k, x);
        emit(&k, I_STORELAST, 0, NULL, -1);
    }
    emit(&k, I_END, 0, NULL, 0);
    return k.c;
}

static Chunk *chunk_for(Node *body) {
    if (!body->code) body->code = compile_body(body);
    return body->code;
}

/* Run a statement list in scope e and return the value of its last statement,
 * matching the tree-walker. A return statement yields retvalue (set by I_RET). */
static Value vm_run(Chunk *c, Env *env) {
    Value *st = alloca((size_t)(c->maxdepth + 4) * sizeof(Value));
    Env *cur = env;
    Value last = nul();
    int sp = 0, pc = 0;
    int saved_depth = loop_depth;
    for (;;) {
        Instr *in = &c->code[pc++];
        if (in->line) g_line = in->line;
        switch ((Op)in->op) {
        case I_CONST:
            st[sp++] = c->consts[in->a];
            break;
        case I_GET:
            st[sp++] = get(cur, in->p);
            break;
        case I_DEFINE:
            define(cur, in->p, st[sp - 1]);
            break;
        case I_SETVAR:
            set(cur, in->p, st[sp - 1]);
            break;
        case I_POP:
            sp--;
            break;
        case I_STORELAST:
            last = st[--sp];
            break;
        case I_BIN:
            sp--;
            st[sp - 1] = binop(in->p, st[sp - 1], st[sp]);
            break;
        case I_UN:
            st[sp - 1] = unop(in->p, st[sp - 1]);
            break;
        case I_JMP:
            pc = in->a;
            break;
        case I_JF:
            if (!truth(st[--sp])) pc = in->a;
            break;
        case I_ANDJ:
            if (!truth(st[sp - 1])) pc = in->a; else sp--;
            break;
        case I_ORJ:
            if (truth(st[sp - 1])) pc = in->a; else sp--;
            break;
        case I_CALL: {
            int argc = in->a;
            Value fn = get(cur, in->p);
            Value r = call(fn, &st[sp - argc], argc, cur);
            sp -= argc;
            st[sp++] = r;
            break;
        }
        case I_EVAL:
            st[sp++] = eval(in->p, cur);
            break;
        case I_SIG:
        case I_BREAK:
        case I_CONT:
            if (in->op != I_SIG) {
                if (loop_depth <= loop_floor) {
                    rt_err(in->op == I_BREAK ? "break used outside a loop\n" : "continue used outside a loop\n");
                    exit(3);
                }
                loop_signal = in->op == I_BREAK ? 1 : 2;
            }
            if (returning) goto done;
            if (loop_signal == 1 || loop_signal == 2) {
                if (in->a < 0) goto done;   /* no enclosing loop here: leave the flag for the caller */
                int kind = loop_signal;
                loop_signal = 0;
                pc = kind == 1 ? c->loops[in->a].brk : c->loops[in->a].cont;
            }
            break;
        case I_RET:
            retvalue = st[--sp];
            returning = true;
            goto done;
        case I_LOOP_BEGIN:
            loop_depth++;
            break;
        case I_LOOP_END:
            loop_depth--;
            break;
        case I_FOR_PREP: {
            Value seq = st[--sp];
            long count;
            if (seq.t == V_LIST) count = (long)seq.list->len;
            else if (seq.t == V_STR) count = (long)seq.len;
            else if (seq.t == V_DICT) count = (long)seq.dict->len;
            else { rt_err("for-in needs a list, string, or dictionary\n"); exit(3); }
            st[sp++] = (Value){.t = V_NULL, .ptr = cur};   /* saved outer scope */
            cur = env_new(cur);
            st[sp++] = seq;
            st[sp++] = num((double)count);
            st[sp++] = num(0);
            loop_depth++;
            break;
        }
        case I_FOR_NEXT: {
            Value *idx = &st[sp - 1];
            size_t i = (size_t)idx->n;
            size_t count = (size_t)st[sp - 2].n;
            Value seq = st[sp - 3];
            if (seq.t == V_STR) {
                /* Text: the index is a byte offset; each step yields one whole character. */
                if (i >= seq.len) { pc = in->a; break; }
                size_t w = utf8_char_width(seq.s, i, seq.len);
                define(cur, in->p, strv_n(seq.s + i, w));
                idx->n = (double)(i + w);
                break;
            }
            size_t len = seq.t == V_LIST ? seq.list->len : seq.dict->len;
            if (i >= count || i >= len) { pc = in->a; break; }
            Value item = seq.t == V_LIST ? seq.list->items[i] : str(seq.dict->keys[i]);
            define(cur, in->p, item);
            idx->n = (double)(i + 1);
            break;
        }
        case I_FOR_END:
            sp -= 3;
            env_release(cur);   /* compiled loops never capture their scope */
            cur = (Env *)st[--sp].ptr;
            loop_depth--;
            break;
        case I_END:
            goto done;
        }
    }
done:
    loop_depth = saved_depth;
    return returning ? retvalue : last;
}

/* Run a function or module body in scope e: compiled, or tree-walked with --tree. */
static Value run_body(Node *body, Env *e) {
    if (g_use_vm) return vm_run(chunk_for(body), e);
    Value r = nul();
    for (Node *x = body->next; x && !returning && !loop_signal; x = x->next) r = eval(x, e);
    return r;
}

static void disasm_chunk(Chunk *c, const char *title) {
    printf("== %s (%d instructions, max stack %d) ==\n", title, c->n, c->maxdepth);
    for (int i = 0; i < c->n; i++) {
        Instr *in = &c->code[i];
        printf("%04d  line %-4d %-11s", i, in->line, op_names[in->op]);
        switch ((Op)in->op) {
        case I_CONST: {
            char *s = stringify(c->consts[in->a]);
            printf(" %s", s);
            break;
        }
        case I_GET: case I_DEFINE: case I_SETVAR: case I_FOR_PREP: case I_FOR_NEXT:
            printf(" %s", (const char *)in->p);
            break;
        case I_CALL:
            printf(" %s/%d", (const char *)in->p, in->a);
            break;
        case I_BIN: case I_UN:
            printf(" %s", ((Node *)in->p)->op);
            break;
        case I_EVAL:
            printf(" tree-walker node %d", (int)((Node *)in->p)->t);
            break;
        case I_JMP: case I_JF: case I_ANDJ: case I_ORJ: case I_FOR_END:
            printf(" -> %04d", in->a);
            break;
        case I_SIG: case I_BREAK: case I_CONT:
            printf(" loop %d", in->a);
            break;
        default:
            break;
        }
        printf("\n");
    }
}
