#ifndef ERR
#define ERR

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

#endif
