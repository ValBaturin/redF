#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <editline/readline.h>

#include "mpc/mpc.h"

#define LASSERT(args, cond, errt, fmt, ...) \
    if (!(cond)) { \
        lval* err = newE(errt, fmt, ##__VA_ARGS__); \
        lval_del(args); \
        return err; \
    }
#define LASSERT_SAFE(cond, errt, fmt, ...) \
    if (!(cond)) { \
        lval* err = newE(errt, fmt, ##__VA_ARGS__); \
        return err; \
    }
#define LASSERT_NUM(where, v, num) \
    if ((v->count) != num) { \
        lval* err = newE(ERR_BAD_OP, "%s passed wrong num of args, expected %i, got %i", \
                where, num, v->count); \
        lval_del(v); \
        return err; \
    }
#define LASSERT_TYPE(where, v, argn, expected) \
    if ((v->cell[argn]->type) != expected) { \
        lval* err = newE(ERR_BAD_OP, "%s passed wrong type as %i arg, expected %s", \
                where, argn, ltype_name(expected)); \
        lval_del(v); \
        return err; \
    }

// Expression type. l is for lis.
// I - Integer
// F - Float
// E - Error
// SY - Symbol
// SE - S-expression
// Q  - Q-expression
// FUN - Function pointer
enum ltype{I, F, E, SY, SE, Q, FUN};

// Error type
enum etype {
    ERR_DIV_ZERO,
    ERR_BAD_OP,
    ERR_BAD_NUM,
    ERR_UNK,
    ERR_NO_ERR,
    ERR_NOT_SEXPR,
    ERR_NOT_QEXPR,
    ERR_NOT_DEFINED,
    ERR_OUT_OF_BOUND,
};

// Symbol types
enum stype {
    ADD,
    SUB,
    MUL,
    DIV,
    LIST,
    HEAD,
    TAIL,
    JOIN,
    EVAL,
    DEF,
    ASSGN,
    LAMBDA,
    CUSTOM,
    UNKNSYM,
    // Special forms
    SPECIAL_QUOTE,
    SPECIAL_SETQ,
    SPECIAL_LAMBDA,
};

// Forward declaration to allow type recursion
typedef struct lenv lenv;
typedef struct lval lval;

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lenv {
    int count;
    lenv* par; // parent environment
    char** syms;
    lval** vals;
};

struct lval {
    enum ltype type;
    // lval value
    union {
        // Number values
        long int in;
        double fn;
        // Err value
        enum etype err;
        // Symbol value
        enum stype sym;
        // func value
        struct {
                lbuiltin builtin; // Points if it is builtin func
                lenv* env;
                lval* params;
                lval* body;
            } funcv;
        } v;
        char* err;
        char* sym;
        int count;
        lval** cell;
};

char* ltype_name(enum ltype type) {
    switch(type) {
        case I: return "Integer number";
        case F: return "Decimal number";
        case E: return "Error";
        case SY: return "Symbol";
        case SE: return "S-expression";
        case Q: return "Q-expression";
        case FUN: return "Function";
        default: return "Unknown expression";
    }
}

lenv* new_lenv() {
    lenv* env = malloc(sizeof(lenv));
    env->par = NULL;
    env->count = 0;
    env->syms = NULL;
    env->vals = NULL;
    return env;
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
    // Parse some symbols to special forms
    else if (strcmp(s, "quote") == 0) { v->v.sym = SPECIAL_QUOTE; }
    else if (strcmp(s, "setq") == 0) { v->v.sym = SPECIAL_SETQ; }
    else if (strcmp(s, "lambda") == 0) { v->v.sym = SPECIAL_LAMBDA; }
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

void lenv_del(lenv* env);

void lval_del(lval* v) {
    switch (v->type) {
        case I:
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

void lenv_del(lenv* env) {
    for (int i = 0; i < env->count; i++) {
        free(env->syms[i]);
        lval_del(env->vals[i]);
    }

    free(env->syms);
    free(env->vals);
    free(env);

    return;
}

lval* lval_read_int(mpc_ast_t* t) {
    errno = 0;
    long int v = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? newI(v)
                           : newE(ERR_BAD_NUM, "invalid number");
}


lval* lval_read_float(mpc_ast_t* t) {
    errno = 0;
    double v = strtod(t->contents, NULL);
    return errno != ERANGE ? newF(v)
                           : newE(ERR_BAD_NUM, "invalid number");

}

lval* lval_read_num(mpc_ast_t* t) {
    if (strstr(t->tag, "integer")) { return lval_read_int(t); }
    if (strstr(t->tag, "decimal")) { return lval_read_float(t); }
    return newE(ERR_UNK, "Tried to parse number out of thin air");
}

lval* lval_add(lval* vs, lval* v) {
    vs->count++;
    vs->cell = realloc(vs->cell, sizeof(lval*) * vs->count);
    vs->cell[vs->count-1] = v;
    return vs;
}

lval* lval_read(mpc_ast_t* t) {
    // Terminals
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return newSY(t->contents); }

    // New branch
    lval* v = NULL;
    if      (strstr(t->tag, "sexpr"))  { v = newSE(); }
    else if (strstr(t->tag, "qexpr"))  { v = newQ();  }
    else if (strcmp(t->tag, ">") == 0) { v = newSE(); }


    // Fill this new branch with contained expressions
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        v = lval_add(v, lval_read(t->children[i]));
    }

    return v;
}

void lval_expr_print(lval* v, char open, char close);

void lval_print(lval* v) {
    switch (v->type) {
        case I: printf("%li", v->v.in); break;
        case F: printf("%lf", v->v.fn); break;
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

lval* lval_copy(lval* v);

lenv* lenv_copy(lenv* env) {
    lenv* nenv = malloc(sizeof(lenv));
    nenv->par = env->par;
    nenv->count = env->count;
    nenv->syms = malloc(sizeof(char*) * nenv->count);
    nenv->vals = malloc(sizeof(lval*) * nenv->count);

    for (int i = 0; i < env->count; i++) {
        nenv->syms[i] = malloc(strlen(env->syms[i]) + 1);
        strcpy(nenv->syms[i], env->syms[i]);
        nenv->vals[i] = lval_copy(env->vals[i]);
    }

    return nenv;
}

lval* lval_copy(lval* v) {
    lval* copy = malloc(sizeof(lval));
    copy->type = v->type;

    switch (v->type) {
        case F: copy->v.fn = v->v.fn; break;
        case I: copy->v.in = v->v.in; break;

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

lval* lenv_get(lenv* env, lval* var) {
    // var must be a symbol
    LASSERT_SAFE(var->type == SY, ERR_BAD_OP,
            "Not a variable name");

    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->syms[i], var->sym) == 0) {
            // TODO: why copy?
            return lval_copy(env->vals[i]);
        }
    }

    if (env->par) {
        return lenv_get(env->par, var);
    } else {
        return newE(ERR_NOT_DEFINED, "Variable %s is not defined", var->sym);
    }

}

// Put var in local (innermost, passed) environment
void lenv_put(lenv* env, lval* var, lval* v) {

    for (int i = 0; i < env->count; i++) {
    // If we found var
        if (strcmp(env->syms[i], var->sym) == 0) {
            lval_del(env->vals[i]);
            env->vals[i] = lval_copy(v);
            return;
        }
    }
    // If we didn't
    env->count++;
    env->vals = realloc(env->vals, sizeof(lval*) * env->count);
    env->syms = realloc(env->syms, sizeof(char*) * env->count);
    env->vals[env->count-1] = lval_copy(v);
    env->syms[env->count-1] = malloc(strlen(var->sym)+1);
    strcpy(env->syms[env->count-1], var->sym);

    return;
}

// Put var in global environment
void lenv_def(lenv* env, lval* var, lval* v) {
    while (env->par) {
        env = env->par;
    }

    lenv_put(env, var, v);
    return;
}

lval* lval_eval_sexpr(lenv* env, lval* se);

lval* builtin_head(lenv* env, lval* vs) {

    LASSERT(vs, vs->count == 1,
        ERR_NOT_QEXPR, "Function 'head' passed too many arguments.");
    LASSERT(vs, vs->cell[0]->type == Q,
        ERR_NOT_QEXPR, "Function 'head' passed incorrect type for argument 0. "
                       "Got %s, Exprected %s.",
                       ltype_name(vs->cell[0]->type), ltype_name(Q));
    LASSERT(vs, vs->cell[0]->count != 0,
        ERR_NOT_QEXPR, "Function 'head' passed {}");


    lval* v = lval_take(vs, 0);

    // TODO: Make it work without while...
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;

}

lval* builtin_tail(lenv* env, lval* vs) {

    LASSERT(vs, vs->count == 1,
        ERR_NOT_QEXPR, "Function 'tail' passed too many arguments.");
    LASSERT(vs, vs->cell[0]->type == Q,
        ERR_NOT_QEXPR, "Function 'tail' passed incorrect type.");
    LASSERT(vs, vs->cell[0]->count != 0,
        ERR_NOT_QEXPR, "Function 'tail' passed {}");

    lval* v = lval_take(vs, 0);
    lval_println(v);
    lval_del(lval_pop(v, 0));
    return v;
}

lval* lval_join(lval* a, lval* b) {

    while (b->count) {
        a = lval_add(b, lval_pop(b, 0));
    }

    lval_del(b);
    return a;
}

lval* builtin_join(lenv* env, lval* vs) {
    for (int i = 0; i < vs->count; i++) {
        LASSERT(vs, vs->cell[i]->type == Q,
            ERR_NOT_QEXPR, "Function 'join' passed incorrect type.")
    }

    lval* v = lval_pop(vs, 0);

    while (vs->count) {
        v = lval_join(v, lval_pop(vs, 0));
    }

    lval_del(vs);
    return v;

}

lval* builtin_var(lenv* env, lval* v, enum stype stype) {
    // LASSERT_TYPE("Var assignment func", v, 0, Q)
    
    // Starting from ver 0.10.0 builtin_var can also be passed special forms
    // and thus should also accept SEs
    LASSERT(v, v->type == SE || v->type == Q, ERR_BAD_OP,
            "Var assignment function passed wrong argument type"
            "Got %s, Expected %s.",
            ltype_name(v->type),
            ltype_name(SY));
    

    lval* syms = v->cell[0];

    for (int i = 0; i < syms->count; i++) {
        LASSERT(v, syms->cell[i]->type == SY, ERR_BAD_OP,
                "Var assignment function cannot define symbol"
                "Got %s, Expected %s.",
                ltype_name(syms->cell[i]->type),
                ltype_name(SY));
    }

    LASSERT(v, syms->count == v->count-1, ERR_BAD_OP,
            "Var assignment function passed unmatched symbols or values,"
            "Got %i, Expected %i.",
            syms->count,
            v->count-1);

    switch (stype) {
        case DEF:
            for (int i = 0; i < syms->count; i++) {
                lenv_def(env, syms->cell[i], v->cell[i+1]);
            }
            break;
        case ASSGN:
            for (int i = 0; i < syms->count; i++) {
                lenv_put(env, syms->cell[i], v->cell[i+1]);
            }
            break;
        default:
            lval_del(v);
            return newE(ERR_BAD_OP, "def func fucked up");
            break;
    }

    lval_del(v);
    return newSE();
}

lval* builtin_def(lenv* env, lval* v) {
    return builtin_var(env, v, DEF);
}

lval* builtin_put(lenv* env, lval* v) {
    return builtin_var(env, v, ASSGN);
}

lval* builtin_lambda(lenv* env, lval* v) {
    LASSERT_NUM("Lambda", v, 2);

    // TODO: multiparam assert macro
    LASSERT(v, v->cell[0]->type == SE || v->cell[0]->type == Q, ERR_BAD_OP,
            "Lambda function passed wrong argument type"
            "Got %s, Expected %s.",
            ltype_name(v->type),
            ltype_name(SY));
    LASSERT(v, v->cell[1]->type == SE || v->cell[1]->type == Q, ERR_BAD_OP,
            "Lambda function passed wrong argument type"
            "Got %s, Expected %s.",
            ltype_name(v->type),
            ltype_name(SY));

    for (int i = 0; i < v->cell[0]->count; i++) {
        LASSERT(v, (v->cell[0]->cell[i]->type == SY), ERR_BAD_OP,
                "Cannot define non-symbol. Got %s, Expected %s.",
                ltype_name(v->cell[0]->cell[i]->type), ltype_name(SY))
    }

    lval* params = lval_pop(v, 0);
    lval* body = lval_pop(v, 0);
    lval_del(v);

    return newLambda(params, body);
}

lval* builtin_list(lenv* env, lval* a) {
    a->type = Q;
    return a;
}

lval* lval_eval(lenv* env, lval* atom);

lval* builtin_eval(lenv* env, lval* vs) {
    LASSERT(vs, vs->count == 1,
            ERR_BAD_OP, "Function 'eval' passed too many arguments.");
    LASSERT(vs, vs->cell[0]->type == Q || vs->cell[0]->type == SE,
            ERR_BAD_OP, "Function 'eval' passed incorrect type.");

    lval* a = lval_take(vs, 0);
    a->type = SE;
    return lval_eval(env, a);
}

// lval* builtin_op(lenv* env, lval* vs, enum stype op);
//
// lval* builtin(lenv* env, lval* a, enum stype st) {
//     switch (st) {
//         case ADD:   { return builtin_op(env, a, st); }
//         case SUB:   { return builtin_op(env, a, st); }
//         case MUL:   { return builtin_op(env, a, st); }
//         case DIV:   { return builtin_op(env, a, st); }
//         case LIST:  { return builtin_list(env, a); }
//         case HEAD:  { return builtin_head(env, a); }
//         case TAIL:  { return builtin_tail(env, a); }
//         case JOIN:  { return builtin_join(env, a); }
//         case EVAL:  { return builtin_eval(env, a); }
//         default :   { lval_del(a); return newE(ERR_BAD_OP, "Unknown function."); }
//     }
// }

    lval* builtin_add(lenv* env, lval* vs) {
        if (vs->count < 2) {
            return newE(ERR_NOT_SEXPR, "Unsupported number of arguments in the S-expression");
        }


        lval* a = lval_pop(vs, 0);
        while (vs->count > 0) {
            lval* b = lval_pop(vs, 0);
            switch (a->type) {
                case I:
                    switch (b->type) {
                        case I: a->v.in += b->v.in; break;
                        case F:
                            a->type = F;
                            a->v.fn = a->v.in;
                            a->v.fn += b->v.fn;
                            break;
                        default:
                            lval_del(a); lval_del(b);
                            lval_del(vs);
                            return newE(ERR_NOT_SEXPR, "Add on NaN operand");
                }
                break;
            case F:
                switch (b->type) {
                    case I:
                        b->type = F;
                        b->v.fn = b->v.in;
                        a->v.fn += b->v.fn;
                        break;
                    case F: a->v.fn += b->v.fn; break;
                    default:
                        lval_del(a); lval_del(b);
                        lval_del(vs);
                        return newE(ERR_NOT_SEXPR, "Add on NaN operand");
                }
                break;
            default:
                lval_del(a); lval_del(b);
                lval_del(vs);
                return newE(ERR_NOT_SEXPR, "Add on NaN operand");
        }
        lval_del(b);
    }

    lval_del(vs);
    return a;
}

lval* builtin_sub(lenv* env, lval* vs) {

    if (vs->count < 1) {
        return newE(ERR_NOT_SEXPR, "Unsupported number of arguments in the S-expression");
    }

    lval* a = lval_pop(vs, 0);

    if (vs->count == 0) {
        switch (a->type) {
            case I: a->v.in = -a->v.in; break;
            case F: a->v.fn = -a->v.fn; break;
            default:
                lval_del(vs);
                lval_del(a);
                return newE(ERR_NOT_SEXPR, "NaN operand in the S-expression");
                break;
        }
    }

    while (vs->count > 0) {
        lval* b = lval_pop(vs, 0);
        switch (a->type) {
            case I:
                switch (b->type) {
                    case I: a->v.in -= b->v.in; break;
                    case F:
                        a->type = F;
                        a->v.fn = a->v.in;
                        a->v.fn -= b->v.fn;
                        break;
                    default:
                        lval_del(a); lval_del(b);
                        lval_del(vs);
                        return newE(ERR_NOT_SEXPR, "Sub on NaN operand");
                }
                break;
            case F:
                switch (b->type) {
                    case I:
                        b->type = F;
                        b->v.fn = b->v.in;
                        a->v.fn -= b->v.fn;
                        break;
                    case F: a->v.fn -= b->v.fn; break;
                    default:
                        lval_del(a); lval_del(b);
                        lval_del(vs);
                        return newE(ERR_NOT_SEXPR, "Sub on NaN operand");
                }
                break;
            default:
                lval_del(a); lval_del(b);
                lval_del(vs);
                return newE(ERR_NOT_SEXPR, "Sub on NaN operand");
        }
        lval_del(b);
    }

    lval_del(vs);
    return a;
}


lval* builtin_mul(lenv* env, lval* vs) {

    if (vs->count < 2) {
        return newE(ERR_NOT_SEXPR, "Unsupported number of arguments in the S-expression");
    }


    lval* a = lval_pop(vs, 0);
    while (vs->count > 0) {
        lval* b = lval_pop(vs, 0);
        switch (a->type) {
            case I:
                switch (b->type) {
                    case I: a->v.in *= b->v.in; break;
                    case F:
                        a->type = F;
                        a->v.fn = a->v.in;
                        a->v.fn *= b->v.fn;
                        break;
                    default:
                        lval_del(a); lval_del(b);
                        lval_del(vs);
                        return newE(ERR_NOT_SEXPR, "Mul on NaN operand");
                }
                break;
            case F:
                switch (b->type) {
                    case I:
                        b->type = F;
                        b->v.fn = b->v.in;
                        a->v.fn *= b->v.fn;
                        break;
                    case F: a->v.fn *= b->v.fn; break;
                    default:
                        lval_del(a); lval_del(b);
                        lval_del(vs);
                        return newE(ERR_NOT_SEXPR, "Mul on NaN operand");
                }
                break;
            default:
                lval_del(a); lval_del(b);
                lval_del(vs);
                return newE(ERR_NOT_SEXPR, "Mul on NaN operand");
        }
        lval_del(b);
    }

    lval_del(vs);
    return a;
}


lval* builtin_div(lenv* env, lval* vs) {

    if (vs->count < 2) {
        return newE(ERR_NOT_SEXPR, "Unsupported number of arguments in the S-expression");
    }


    lval* a = lval_pop(vs, 0);
    while (vs->count > 0) {
        lval* b = lval_pop(vs, 0);
        switch (a->type) {
            case I:
                switch (b->type) {
                    case I:
                        if (b->v.in == 0) {
                            lval_del(a); lval_del(b);
                            lval_del(vs);
                            return newE(ERR_DIV_ZERO,"");
                        }
                        a->v.in /= b->v.in;
                        break;
                    case F:
                        a->type = F;
                        a->v.fn = a->v.in;
                        if (b->v.fn == 0) {
                            lval_del(a); lval_del(b);
                            lval_del(vs);
                            return newE(ERR_DIV_ZERO,"");
                        }
                        a->v.fn /= b->v.fn;
                        break;
                    default:
                        lval_del(a); lval_del(b);
                        lval_del(vs);
                        return newE(ERR_NOT_SEXPR, "Div on NaN operand");
                }
                break;
            case F:
                switch (b->type) {
                    case I:
                        b->type = F;
                        b->v.fn = b->v.in;
                        if (b->v.fn == 0) {
                            lval_del(a); lval_del(b);
                            lval_del(vs);
                            return newE(ERR_DIV_ZERO,"");
                        }
                        a->v.fn /= b->v.fn;
                        break;
                    case F:
                        if (b->v.fn == 0) {
                            lval_del(a); lval_del(b);
                            lval_del(vs);
                            return newE(ERR_DIV_ZERO,"");
                        }
                        a->v.fn /= b->v.fn;
                        break;
                    default:
                        lval_del(a); lval_del(b);
                        lval_del(vs);
                        return newE(ERR_NOT_SEXPR, "Div on NaN operand");
                }
                break;
            default:
                lval_del(a); lval_del(b);
                lval_del(vs);
                return newE(ERR_NOT_SEXPR, "Div on NaN operand");
        }
        lval_del(b);
    }

    lval_del(vs);
    return a;
}

lval* lval_eval(lenv* env, lval* atom) {
    // TODO: Resolve predefined symbols here?
    if (atom->type == SY) {
        switch(atom->v.sym) {
            case ADD:  lval_del(atom); return newFUN(builtin_add);
            case SUB:  lval_del(atom); return newFUN(builtin_sub);
            case MUL:  lval_del(atom); return newFUN(builtin_mul);
            case DIV:  lval_del(atom); return newFUN(builtin_div);
            case LIST:  lval_del(atom); return newFUN(builtin_list);
            case HEAD:  lval_del(atom); return newFUN(builtin_head);
            case TAIL:  lval_del(atom); return newFUN(builtin_tail);
            case JOIN: lval_del(atom); return newFUN(builtin_join);
            case EVAL: lval_del(atom); return newFUN(builtin_eval);
            case LAMBDA: lval_del(atom); return newFUN(builtin_lambda);
            case DEF: lval_del(atom); return newFUN(builtin_def);
            case ASSGN: lval_del(atom); return newFUN(builtin_put);
            // Specail forms remain to be a symbol and then get resolved in a different way
            case SPECIAL_QUOTE:
            case SPECIAL_SETQ:
            case SPECIAL_LAMBDA:
                return atom;
            default:; // ; hack - https://stackoverflow.com/questions/18496282/why-do-i-get-a-label-can-only-be-part-of-a-statement-and-a-declaration-is-not-a
                lval* v = lenv_get(env, atom);
                lval_del(atom);
                return v;

        }
    }

    if (atom->type == SE) { return lval_eval_sexpr(env, atom); }

    return atom;
}

void env_print(lenv* env) {
    puts("env_print");
    for (int i = 0; i < env->count; i++) {
        printf("sym - %s, lval - %s\n", env->syms[i], ltype_name(env->vals[i]->type));
    }
    if (env->par) {
        puts("Have par");
        env_print(env->par);
    }
    return;
}

lval* lval_call(lenv* env, lval* f, lval* v) {
    // If we call builtin function, just return it
    if (f->v.funcv.builtin) { return f->v.funcv.builtin(env, v); }

    int given = v->count;
    int total = f->v.funcv.params->count;

    // Recursive partial function application
    while (v->count) {
        if (f->v.funcv.params->count == 0) {
            lval_del(v);
            return newE(ERR_BAD_OP,
                    "Function passed too many arguments"
                    "Got %i, Expected %i.", given, total);
        }

        lval* sym = lval_pop(f->v.funcv.params, 0);

        // Mulstiparams special character
        if (strcmp(sym->sym, "&") == 0) {

            if (f->v.funcv.params->count != 1) {
                lval_del(v);
                return newE(ERR_BAD_OP,
                        "Function format invalid"
                        "Symbos '&' must be followed by a single symbol");
            }

            lval* nsym = lval_pop(f->v.funcv.params, 0);
            lenv_put(f->v.funcv.env, nsym, builtin_list(env, v));
            lval_del(sym); lval_del(nsym);
            break;
        }
        lval* val = lval_pop(v, 0);
        lenv_put(f->v.funcv.env, sym, val);

        lval_del(sym); lval_del(val);
    }

    lval_del(v);

    // check {xs & ()} case
    if (f->v.funcv.params->count > 0 &&
            strcmp(f->v.funcv.params->cell[0]->sym, "&") == 0) {
        if (f->v.funcv.params->count != 2) {
            return newE(ERR_BAD_OP,
                    "Function format invalid"
                    "Symbol '&' must be followed by single symbol");
        }

        lval_del(lval_pop(f->v.funcv.params, 0));

        lval* sym = lval_pop(f->v.funcv.params, 0);
        lval* val = newQ();

        lenv_put(f->v.funcv.env, sym, val);
        lval_del(sym); lval_del(val);
    }

    // Full bound func is to evaluate
    if (f->v.funcv.params->count == 0) {
        f->v.funcv.env->par = env;

        return builtin_eval(f->v.funcv.env,
                lval_add(newSE(), lval_copy(f->v.funcv.body)));
    // Partially appliead func returned as is
    } else {
        return lval_copy(f);
    }

    return newE(ERR_BAD_OP, "lval_call reached end of function");
}

lval* lval_eval_special_sexpr(lenv* env, lval* se) {

    // We don't assert anything here as we suppose that
    // this function is called only after all asserts needed

    // First symbol (exactly symbol) is a special form symbol
    lval* sf = lval_pop(se, 0);

    switch (sf->v.sym) {
        case SPECIAL_QUOTE:
            lval_del(sf);
            // builtin_list ia a good solution to create Q-expression
            return builtin_list(env, se);
        case SPECIAL_SETQ:
            lval_del(sf);
            // Actually many builtin funcs
            // are good solutions to eval special forms
            return builtin_var(env, se, DEF);
        case SPECIAL_LAMBDA:
            lval_del(sf);
            return builtin_lambda(env, se);

        default: goto EXIT_EVAL_SPECIAL_SEXPR;
    }

EXIT_EVAL_SPECIAL_SEXPR:
    lval_del(sf);
    return newE(ERR_BAD_OP, "Unknown special form");

}

lval* lval_eval_sexpr(lenv* env, lval* se) {

    if (se->count == 0) { return se; }

    for (int i = 0; i < se->count; i++) {
        se->cell[i] = lval_eval(env, se->cell[i]);
        // Is it still a symbol? Evaluate it as a special form
        if (i == 0 && se->cell[i]->type == SY) {
            return lval_eval_special_sexpr(env, se);
        }

    }


    for (int i = 0; i < se->count; i++) {
        if (se->cell[i]->type == E) {
            return lval_take(se, i);
        }
    }

    if (se->count == 1) { return lval_take(se, 0); }

    lval* func = lval_pop(se, 0);
    if (func->type != FUN) {
        lval* err = newE(ERR_NOT_SEXPR, "S-expression Does not start with function, but with %s.", ltype_name(func->type));
        lval_del(func);
        lval_del(se);
        return err;
    }

    lval* result = lval_call(env, func, se);
    lval_del(func);

    return result;
}

void lenv_add_builtin(lenv* env, char* name, lbuiltin func) {
    lval* sym = newSY(name);
    lval* v   = newFUN(func);
    lenv_put(env, sym, v);
    lval_del(sym); lval_del(v);

    return;
}

void lenv_add_builtins(lenv* env) {
    lenv_add_builtin(env, "+", builtin_add);
    lenv_add_builtin(env, "-", builtin_sub);
    lenv_add_builtin(env, "*", builtin_mul);
    lenv_add_builtin(env, "/", builtin_div);
    lenv_add_builtin(env, "list", builtin_list);
    lenv_add_builtin(env, "head", builtin_head);
    lenv_add_builtin(env, "tail", builtin_tail);
    lenv_add_builtin(env, "join", builtin_join);
    lenv_add_builtin(env, "eval", builtin_eval);
    lenv_add_builtin(env, "\\", builtin_lambda);
    lenv_add_builtin(env, "def", builtin_def);
    lenv_add_builtin(env, "=", builtin_put);
}

int main(int argc, char** argv) {

    // Create parsers
    mpc_parser_t* Integer = mpc_new("integer");
    mpc_parser_t* Decimal = mpc_new("decimal");
    mpc_parser_t* Number  = mpc_new("number");
    mpc_parser_t* Symbol  = mpc_new("symbol");
    mpc_parser_t* Expr    = mpc_new("sexpr");
    mpc_parser_t* Qexpr   = mpc_new("qexpr");
    mpc_parser_t* Sexpr   = mpc_new("expr");
    mpc_parser_t* Program = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                               \
                integer     : /-?[0-9]+/ ;                                  \
                decimal     : /-?[0-9]+\\.[0-9]+/ ;                         \
                number      : <decimal> | <integer> ;                       \
                symbol      : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;            \
                qexpr       : '{' <expr>* '}' ;                             \
                sexpr       : '(' <expr>* ')' ;                             \
                expr        : <number> | <symbol> | <sexpr> | <qexpr> ;     \
                program     : /^/ <expr>* /$/ ;                             \
            ",
            Integer, Decimal, Number, Symbol, Qexpr, Sexpr, Expr, Program);



    // Print Version and Exit Informaton
    puts("lis v0.10.0");
    puts("Press ^C to Exit\n");

    lenv* env = new_lenv();
    lenv_add_builtins(env);

    while(1) {
        mpc_result_t r;

        char* input = readline("> ");
        // Attempt to Parse the user Input
        if (mpc_parse("<stdin>", input, Program, &r)) {
//            puts("AST:");
//            mpc_ast_print(r.output);
            lval_println(lval_read(r.output));
            puts("\nevaluation:");
            lval* x = lval_eval(env, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        add_history(input);
        free(input);

    }

    mpc_cleanup(8, Number, Decimal, Integer, Symbol, Qexpr, Sexpr, Expr, Program);

    return 0;
}
