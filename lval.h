#ifndef LVAL
#define LVAL

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct lval lval;
typedef struct lenv lenv;
typedef lval*(*lbuiltin)(lenv*, lval*);
enum stype;
enum ltype;

#include "ast.h"
#include "lenv.h"
#include "builtin.h"
#include "err.h"

// Expression type. l is for lis.
// I - Integer
// F - Float
// E - Error
// SY - Symbol
// SE - S-expression
// Q  - Q-expression
// FUN - Function pointer
enum ltype{I, F, E, SY, SE, Q, FUN};

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

lval* newI(long x);

lval* newF(double x);

lval* newE(enum etype e, char* fmt, ...);

lval* newSY(char* s);

lval* newSE();

lval* newQ();

lval* newFUN(lbuiltin func);

void lval_print(lval* v);

void lval_println(lval* v);

lval* newLambda(lval* params, lval* body);

void lval_del(lval* v);

lval* lval_add(lval* vs, lval* v);

lval* lval_read(ast_node* t);

void lval_print(lval* v);

lval new_lval(enum ltype type);

lval* lval_pop(lval* vs, int i);

lval* lval_take(lval* vs, int i);

lval* lval_copy(lval* v);

#endif
