#include "builtin.h"
#include "lval.h"

lval* builtin_head(lenv* env, lval* vs) {

    LASSERT(vs, vs->count == 1,
        ERR_NOT_QEXPR, "Function 'head' passed too many arguments.");
    LASSERT(vs, vs->cell[0]->type == Q || vs->cell[0]->type == SE,
        ERR_NOT_QEXPR, "Function 'head' passed incorrect type for argument 0. "
                       "Got %s, Exprected %s or %s.",
                       ltype_name(vs->cell[0]->type), ltype_name(Q), ltype_name(SE));
    LASSERT(vs, vs->cell[0]->count != 0,
        ERR_NOT_QEXPR, "Function 'head' passed {} (empty list)");


    lval* v = lval_take(vs, 0);

    // TODO: Make it work without while...
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;

}

lval* builtin_tail(lenv* env, lval* vs) {

    LASSERT(vs, vs->count == 1,
        ERR_NOT_QEXPR, "Function 'tail' passed too many arguments.");
    LASSERT(vs, vs->cell[0]->type == Q || vs->cell[0]->type == SE,
        ERR_NOT_QEXPR, "Function 'tail' passed incorrect type for argument 0. "
                       "Got %s, Exprected %s or %s.",
                       ltype_name(vs->cell[0]->type), ltype_name(Q), ltype_name(SE));
    LASSERT(vs, vs->cell[0]->count != 0,
        ERR_NOT_QEXPR, "Function 'tail' passed {} (empty list)");

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
    LASSERT(v, v->cell[1]->type == SE, ERR_BAD_OP,
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

lval* builtin_eval(lenv* env, lval* vs) {
    LASSERT(vs, vs->count == 1,
            ERR_BAD_OP, "Function 'eval' passed too many arguments.");
//    LASSERT(vs, vs->cell[0]->type == Q || vs->cell[0]->type == SE,
//            ERR_BAD_OP, "Function 'eval' passed incorrect type %s.", ltype_name(vs->cell[0]->type));

    lval* ret = lval_take(vs, 0);
    switch (ret->type) {
        case Q:
            ret->type = SE;
            return lval_eval(env, ret);
        case SE:
            return lval_eval(env, ret);
        default: return ret;
    }
}

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

lval* builtin_ord(lenv* env, lval* v, enum stype op) {
    LASSERT_NUM(op, v, 2);
    LASSERT_TYPE("builtin_ord", v, 0, I);
    LASSERT_TYPE("builtin_ord", v, 1, I);

    int r;
    switch (op) {
        case GT: r = (v->cell[0]->v.in > v->cell[1]->v.in); break;
        case LT: r = (v->cell[0]->v.in < v->cell[1]->v.in); break;
        case GE: r = (v->cell[0]->v.in >= v->cell[1]->v.in); break;
        case LE: r = (v->cell[0]->v.in <= v->cell[1]->v.in); break;
        default: lval_del(v); return newE(ERR_BAD_OP, "Wrong comparation operator");
    }
    lval_del(v);
    return newI(r);
}

lval* builtin_gt(lenv* env, lval* v) {
    return builtin_ord(env, v, GT);
}

lval* builtin_lt(lenv* env, lval* v) {
    return builtin_ord(env, v, LT);
}

lval* builtin_ge(lenv* env, lval* v) {
    return builtin_ord(env, v, GE);
}

lval* builtin_le(lenv* env, lval* v) {
    return builtin_ord(env, v, LE);
}

bool lval_eq(lval* a, lval* b) {
    if (a->type != b->type) { return 0; }

    switch (a->type) {
        case I: return (a->v.in == b->v.in);
        case F: return (a->v.fn == b->v.fn);

        case B: return (a->v.b == b->v.b);
        case N: return true;

        case E: return (strcmp(a->err, b->err) == 0);
        case SY: return (strcmp(a->sym, b->sym) == 0);

        case FUN:
            if (a->v.funcv.builtin || b->v.funcv.builtin) {
                return a->v.funcv.builtin == b->v.funcv.builtin;
            } else {
                return lval_eq(a->v.funcv.params, b->v.funcv.params)
                    && lval_eq(a->v.funcv.body, b->v.funcv.body);
            }

        case SE:
        case Q:
            if (a->count != b->count) { return 0; }
            for (int i = 0; i < a->count; i++) {
                if (!lval_eq(a->cell[i], b->cell[i])) { return false; }
            }
            return true;
        break;
    }
    return false;
}

lval* builtin_cmp(lenv* env, lval* v, enum stype op) {
    LASSERT_NUM(op, v, 2);
    bool b;
    switch (op) {
        case EQ: b = lval_eq(v->cell[0], v->cell[1]); break;
        case NE: b = !lval_eq(v->cell[0], v->cell[1]); break;
        default: lval_del(v); return newE(ERR_BAD_OP, "builtin_cmp fucked up");
    }
    lval_del(v);
    return newB(b);
}

lval* builtin_eq(lenv* env, lval* v) {
    return builtin_cmp(env, v, EQ);
}

lval* builtin_ne(lenv* env, lval* v) {
    return builtin_cmp(env, v, NE);
}

lval* builtin_cond(lenv* env, lval* v) {
    LASSERT_NUM("cond", v, 3);
    LASSERT(v, v->count == 2 || v->count == 3, ERR_BAD_OP,
            "cond function passed wrong number of arg");
    LASSERT_TYPE("cond", v, 0, B);
// Any type should be fine starting from 0.13.0
//
//    LASSERT(v, v->cell[1]->type == SE || v->cell[0]->type == Q, ERR_BAD_OP,
//            "cond function passed wrong argument type");
//    LASSERT(v, v->cell[2]->type == SE || v->cell[1]->type == Q, ERR_BAD_OP,
//            "cond function passed wrong argument type");

    lval* ret;

    if (v->cell[1]->type == Q) {
        v->cell[1]->type = SE;
    };

    if (v->count == 3) {
        if (v->cell[2]->type == Q) {
            v->cell[2]->type = SE;
        };
    }

    if (v->cell[0]->v.b) {
        ret = lval_eval(env, lval_pop(v, 1));
    } else {
        if (v->count == 3) {
            ret = lval_eval(env, lval_pop(v, 2));
        } else {
            ret = newN();
        }
    }

    lval_del(v);
    return ret;
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
