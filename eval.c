#include "eval.h"

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
            //
            // As SetQ args passed not evaluated, do it now
            for (int i = 1; i < se->count; i++) {
                se->cell[i] = lval_eval(env, se->cell[i]);
            }
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
