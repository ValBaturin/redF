#ifndef BUILTIN
#define BUILTIN


#include "lval.h"
#include "lenv.h"
#include "err.h"
#include "eval.h"

lval* builtin_head(lenv* env, lval* vs);

lval* builtin_tail(lenv* env, lval* vs);

lval* builtin_join(lenv* env, lval* vs);

lval* builtin_var(lenv* env, lval* v, enum stype stype);

lval* builtin_def(lenv* env, lval* v);

lval* builtin_put(lenv* env, lval* v);

lval* builtin_lambda(lenv* env, lval* v);

lval* builtin_list(lenv* env, lval* a);

lval* builtin_eval(lenv* env, lval* vs);

lval* builtin_add(lenv* env, lval* vs);

lval* builtin_sub(lenv* env, lval* vs);

lval* builtin_mul(lenv* env, lval* vs);

lval* builtin_div(lenv* env, lval* vs);

void lenv_add_builtin(lenv* env, char* name, lbuiltin func);

void lenv_add_builtins(lenv* env);

#endif
