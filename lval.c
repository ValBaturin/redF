#include "lval.h"

char* ltype_name(enum ltype type) {
    switch(type) {
        case I: return "Integer number";
        case F: return "Decimal number";
        case E: return "Error";
        case SY: return "Symbol";
        case SE: return "S-expression";
        case Q: return "Q-expression";
        case FUN: return "Function";
        case B: return "Bool";
        case N: return "Nil";
        default: return "Unknown expression";
    }
}

lval* newI(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = I;
    v->v.in = x;
    return v;
}

lval* newF(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = F;
    v->v.fn = x;
    return v;
}

lval* newE(enum etype e, char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = E;
    v->v.err = e;

    v->err = malloc(513);

    va_list va;
    va_start(va, fmt);

    vsnprintf(v->err, 512, fmt, va);

    v->err = realloc(v->err, strlen(v->err)+1);

    return v;
}

lval* newSY(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = SY;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    if (strcmp(s, "+") == 0) { v->v.sym = ADD; }
    else if (strcmp(s, "-") == 0) { v->v.sym = SUB; }
    else if (strcmp(s, "*") == 0) { v->v.sym = MUL; }
    else if (strcmp(s, "/") == 0) { v->v.sym = DIV; }
    else if (strcmp(s, "list") == 0) { v->v.sym = LIST; }
    else if (strcmp(s, "head") == 0) { v->v.sym = HEAD; }
    else if (strcmp(s, "tail") == 0) { v->v.sym = TAIL; }
    else if (strcmp(s, "join") == 0) { v->v.sym = JOIN; }
    else if (strcmp(s, "eval") == 0) { v->v.sym = EVAL; }
    else if (strcmp(s, "\\") == 0) { v->v.sym = LAMBDA; }
    else if (strcmp(s, "def") == 0) { v->v.sym = DEF; }
    else if (strcmp(s, "gt") == 0) { v->v.sym = GT; }
    else if (strcmp(s, "lt") == 0) { v->v.sym = LT; }
    else if (strcmp(s, "ge") == 0) { v->v.sym = GE; }
    else if (strcmp(s, "le") == 0) { v->v.sym = LE; }
    else if (strcmp(s, "ne") == 0) { v->v.sym = NE; }
    else if (strcmp(s, "eq") == 0) { v->v.sym = EQ; }
    // Parse some symbols to special forms
    else if (strcmp(s, "quote") == 0) { v->v.sym = SPECIAL_QUOTE; }
    else if (strcmp(s, "'") == 0) { v->v.sym = SPECIAL_QUOTE; }
    else if (strcmp(s, "setq") == 0) { v->v.sym = SPECIAL_SETQ; }
    else if (strcmp(s, "lambda") == 0) { v->v.sym = SPECIAL_LAMBDA; }
    else if (strcmp(s, "cond") == 0) { v->v.sym = SPECIAL_COND; }
    else { v->v.sym = CUSTOM; }
    return v;
}

lval* newSE() {
    lval* v = malloc(sizeof(lval));
    v->type = SE;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* newQ() {
    lval* v = malloc(sizeof(lval));
    v->type = Q;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* newFUN(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = FUN;
    v->count = 0;
    v->cell = NULL;
    v->v.funcv.builtin = func;
    return v;
}

lval* newB(bool b) {
    lval* v = malloc(sizeof(lval));
    v->v.b = b;
    v->type = B;
    return v;
}

lval* newN() {
    lval* v = malloc(sizeof(lval));
    v->type = N;
    return v;
}

void lval_print(lval* v);
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* newLambda(lval* params, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = FUN;

    v->v.funcv.builtin = NULL;

    v->v.funcv.env = new_lenv();

    v->v.funcv.params = params;
    v->v.funcv.body = body;

    return v;
}

void lval_del(lval* v) {
    switch (v->type) {
        case I:
        case B:
        case N:
        case F: break;

        case E: free(v->err); break;
        case SY: free(v->sym); break;

        case FUN:
            if (!v->v.funcv.builtin) {
                lenv_del(v->v.funcv.env);
                lval_del(v->v.funcv.params);
                lval_del(v->v.funcv.body);
            }
            break;

        case SE:
        case Q:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }

    free(v);
}

lval* lval_add(lval* vs, lval* v) {
    vs->count++;
    vs->cell = realloc(vs->cell, sizeof(lval*) * vs->count);
    vs->cell[vs->count-1] = v;
    return vs;
}

lval* lval_read(ast_node* t) {
    if (t->type == AST_REAL) { return newF(t->value.r); }
    if (t->type == AST_INT) { return newI(t->value.i); }
    if (t->type == AST_ATOM) { return newSY(t->value.a); }
    if (t->type == AST_BOOL) { return newB(t->value.b); }
    if (t->type == AST_NULL) { return newN(); }
    // TODO add support for the rest of types

    if (t->type == AST_LIST) {
        lval* v = NULL;
        v = newSE();

        for (int i = t->value.children.size - 1; i >= 0; --i) {
            v = lval_add(v, lval_read(t->value.children.nodes[i]));
        }

        return v;
    }
    return NULL;
}

void lval_expr_print(lval* v, char open, char close);

void lval_print(lval* v) {
    switch (v->type) {
        case I: printf("%li", v->v.in); break;
        case F: printf("%lf", v->v.fn); break;
        case B: printf("%s", v->v.b ? "true" : "false"); break;
        case N: printf("nil"); break;
        case SY: printf("%s", v->sym); break;
        case SE: lval_expr_print(v, '(', ')'); break;
        case Q:  lval_expr_print(v, '{', '}'); break;
        case FUN:
            if (v->v.funcv.builtin) {
                printf("<builtin>");
            } else {
                printf("(\\ "); lval_print(v->v.funcv.params);
                putchar(' '); lval_print(v->v.funcv.body); putchar(')');
            }
            break;
        case E:
            if (strcmp(v->err, "") == 0) {
                switch (v->v.err) {
                    case ERR_DIV_ZERO:  printf("Error: Division By Zero."); break;
                    case ERR_BAD_OP:    printf("Error: Invalid Operator."); break;
                    case ERR_BAD_NUM:   printf("Error: Invalid Number."); break;
                    case ERR_UNK:       printf("Error: Unknown Error."); break;
                    case ERR_OUT_OF_BOUND:    printf("Error: Index out of bound."); break;
                    case ERR_NOT_SEXPR:       printf("Error: Not S-expression."); break;
                    case ERR_NOT_QEXPR:       printf("Error: Not Q-expression."); break;
                    case ERR_NOT_DEFINED:     printf("Error: Symbol not defined"); break;
                    case ERR_NO_ERR:    printf("Error: Error not set,\
                            possibly everything is ok."); break;
                }
            } else {
                printf("Error: %s", v->err);
            }
            break;
    }
}

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        lval_print(v->cell[i]);

        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}


lval new_lval(enum ltype type) {
    lval v;
    v.type = type;
    switch (type) {
        case I: v.v.in=0; break;
        case F: v.v.fn=0.; break;
        case E: v.v.err=ERR_NO_ERR; break;
        default: return v;
    };

    return v;
}


lval* lval_pop(lval* vs, int i) {
//    LASSERT(vs, vs->type == SE, ERR_NOT_SEXPR,
//            "lval_pop passed not S-expression");
//    LASSERT(vs, i < vs->count && i >= 0, ERR_OUT_OF_BOUND,
//            "lval_pop invoked with out of bound index");

    lval* v = vs->cell[i];

    memmove(&vs->cell[i], &vs->cell[i+1],
        sizeof(lval*) * (vs->count-i-1));

    vs->count--;

    vs->cell = realloc(vs->cell, sizeof(lval*) * vs->count);

    return v;
}

lval* lval_take(lval* vs, int i) {
//    LASSERT(vs, vs->type == SE, ERR_NOT_SEXPR,
//            "lval_take passed not S-expression");
//    LASSERT(vs, i < vs->count && i >= 0, ERR_OUT_OF_BOUND,
//            "lval_take invoked with out of bound index");

     lval* v = lval_pop(vs, i);

     lval_del(vs);

     return v;
}

lval* lval_copy(lval* v) {
    lval* copy = malloc(sizeof(lval));
    copy->type = v->type;

    switch (v->type) {
        case F: copy->v.fn = v->v.fn; break;
        case I: copy->v.in = v->v.in; break;
        case B: copy->v.b = v->v.b; break;
        case N: break;

        case E:
            copy->err = malloc(strlen(v->err) + 1);
            strcpy(copy->err, v->err);
            break;

        case SY:
            copy->sym = malloc(strlen(v->sym) + 1);
            copy->v.sym = v->v.sym;
            strcpy(copy->sym, v->sym);
            break;

        case SE:
        case Q:
            copy->count = v->count;
            copy->cell = malloc(sizeof(lval*) * copy->count);
            for (int i = 0; i < copy->count; i++) {
                copy->cell[i] = lval_copy(v->cell[i]);
            }
            break;

        case FUN:
            if (v->v.funcv.builtin) {
                copy->v.funcv.builtin = v->v.funcv.builtin;
            } else {
                copy->v.funcv.builtin = NULL;
                copy->v.funcv.env = lenv_copy(v->v.funcv.env);
                copy->v.funcv.params = lval_copy(v->v.funcv.params);
                copy->v.funcv.body = lval_copy(v->v.funcv.body );

            }
            break;
    }

    return copy;
}

