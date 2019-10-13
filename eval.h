#ifndef _EVAL
#define _EVAL

#include "lval.h"
#include "lenv.h"
#include "builtin.h"

lval* lval_eval(lenv* env, lval* atom);

lval* lval_eval_sexpr(lenv* env, lval* se);

lval* lval_call(lenv* env, lval* f, lval* v);

lval* lval_eval_special_sexpr(lenv* env, lval* se);

#endif
