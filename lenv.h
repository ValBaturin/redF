#ifndef LENV
#define LENV

#include "lval.h"

struct lenv {
    int count;
    lenv* par; // parent environment
    char** syms;
    lval** vals;
};

char* ltype_name(enum ltype type);

lenv* new_lenv();

void lenv_del(lenv* env);

lenv* lenv_copy(lenv* env);

lval* lenv_get(lenv* env, lval* var);

void lenv_put(lenv* env, lval* var, lval* v);

void lenv_def(lenv* env, lval* var, lval* v);

void env_print(lenv* env);

#endif
