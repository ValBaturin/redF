#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc/mpc.h"


// Expression type. l is for lis.
// I - Integer
// F - Float
// E - Error
// SY - Symbol
// SE - S-expression
enum ltype{I, F, E, SY, SE};

// Error type
enum etype {
    ERR_DIV_ZERO,
    ERR_BAD_OP,
    ERR_BAD_NUM,
    ERR_UNK,
    ERR_NO_ERR,
    ERR_NOT_SEXPR,
    ERR_OUT_OF_BOUND,
};

enum stype {
    PLUS,
    MINUS,
    MUL,
    DIV,
};

typedef struct lval {
    enum ltype type;
    union {
        long int in;
        double fn;
        enum etype err;
        enum stype sym;
    } v;
    char* err;
    char* sym;
    int count;
    struct lval** cell;
} lval;

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

lval* newE(enum etype e, char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = E;
    v->err = malloc(strlen(s) + 1);
    strcpy(v->err, s);
    v->v.err = e;
    return v;
}

lval* newSY(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = SY;
    v->sym = malloc(strlen(s) + 1);
    if (strcmp(s, "+") == 0) { v->v.sym = PLUS; }
    if (strcmp(s, "-") == 0) { v->v.sym = MINUS; }
    if (strcmp(s, "*") == 0) { v->v.sym = MUL; }
    if (strcmp(s, "/") == 0) { v->v.sym = DIV; }
    strcpy(v->sym, s);
    return v;
}

lval* newSE() {
    lval* v = malloc(sizeof(lval));
    v->type = SE;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {
    switch (v->type) {
        case I:
        case F: break;

        case E: free(v->err); break;
        case SY: free(v->sym); break;

        case SE:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }

    free(v);
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
    if (strstr(t->tag, "sexpr")) { v = newSE(); }
    else if (strcmp(t->tag, ">") == 0) {
        puts("It's strange! We shouldn't fall here... Anyway..." );
        v = newSE();
    }


    // Fill this new branch with contained expressions
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
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
        case E:
            if (strcmp(v->err, "") == 0) {
                switch (v->v.err) {
                    case ERR_DIV_ZERO:  printf("Error: Division By Zero."); break;
                    case ERR_BAD_OP:    printf("Error: Invalid Operator."); break;
                    case ERR_BAD_NUM:   printf("Error: Invalid Number."); break;
                    case ERR_UNK:       printf("Error: Unknown Error."); break;
                    case ERR_OUT_OF_BOUND:       printf("Error: Index out of bound."); break;
                    case ERR_NOT_SEXPR:       printf("Error: Not S-expression."); break;
                    case ERR_NO_ERR:    printf("Error: Error not set,\
                            possibly everything is ok."); break;
                }
            } else {
                printf("Error: %s", v->err);
            }
            break;
    }
}


void lval_println(lval* v) { lval_print(v); putchar('\n'); }


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



lval evalop(lval a, char* op, lval b) {
    if (a.type == E) { return a; }
    if (b.type == E) { return b; }

    // Integer op Integer
    if (a.type == I) {
        if (b.type == I) {
            if (strcmp(op, "+") == 0) { a.v.in += b.v.in; return a; }
            if (strcmp(op, "-") == 0) { a.v.in -= b.v.in; return a; }
            if (strcmp(op, "*") == 0) { a.v.in *= b.v.in; return a; }
            if (strcmp(op, "/") == 0) {
                if (b.v.in == 0) { a.type = E; a.v.err = ERR_DIV_ZERO; return a; }
                else             { a.v.in /= b.v.in; return a; }
            }
            a.type = E;
            a.v.err = ERR_BAD_OP;
            return a;

    // Integer op Float
        } else if (b.type == F) {
            a.type = F;
            a.v.fn = a.v.in;
            if (strcmp(op, "+") == 0) { a.v.fn += b.v.fn; return a; }
            if (strcmp(op, "-") == 0) { a.v.fn -= b.v.fn; return a; }
            if (strcmp(op, "*") == 0) { a.v.fn *= b.v.fn; return a; }
            if (strcmp(op, "/") == 0) {
                if (b.v.in == 0) { a.type = E; a.v.err = ERR_DIV_ZERO; return a; }
                else             { a.v.fn /= b.v.fn; return a; }
            }
            a.type = E;
            a.v.err = ERR_BAD_OP;
            return a;
        }
    // Float op Integer
    } else if (a.type == F) {
        if (b.type == I) {
            b.type = F;
            b.v.fn = b.v.in;
            if (strcmp(op, "+") == 0) { a.v.fn += b.v.fn; return a; }
            if (strcmp(op, "-") == 0) { a.v.fn -= b.v.fn; return a; }
            if (strcmp(op, "*") == 0) { a.v.fn *= b.v.fn; return a; }
            if (strcmp(op, "/") == 0) {
                if (b.v.in == 0) { a.type = E; a.v.err = ERR_DIV_ZERO; return a; }
                else             { a.v.fn /= b.v.fn; return a; }
            }
            a.type = E;
            a.v.err = ERR_BAD_OP;
            return a;
    // Float op Float
        } else if (b.type == F) {
            if (strcmp(op, "+") == 0) { a.v.fn += b.v.fn; return a; }
            if (strcmp(op, "-") == 0) { a.v.fn -= b.v.fn; return a; }
            if (strcmp(op, "*") == 0) { a.v.fn *= b.v.fn; return a; }
            if (strcmp(op, "/") == 0) {
                if (b.v.in == 0) { a.type = E; a.v.err = ERR_DIV_ZERO; return a; }
                else             { a.v.fn /= b.v.fn; return a; }
            }
            a.type = E;
            a.v.err = ERR_BAD_OP;
            return a;
        }
    }
    a.type = E;
    a.v.err = ERR_BAD_NUM;
    return a;
}


lval* lval_pop(lval* vs, int i) {
    if (vs->type != SE) {
        return newE(ERR_NOT_SEXPR, "lval_pop invoked with not an S-expression");
    }
    if (i >= vs->count) {
        return newE(ERR_OUT_OF_BOUND, "lval_pop invoked with out of bound index");
    }

    lval* v = vs->cell[i];

    memmove(&vs->cell[i], &vs->cell[i+1],
        sizeof(lval*) * (vs->count-i-1));

    vs->count--;

    vs->cell = realloc(vs->cell, sizeof(lval*) * vs->count);

    return v;
}

lval* lval_take(lval* vs, int i) {
    if (vs->type != SE) {
        return newE(ERR_NOT_SEXPR, "lval_pop invoked with not an S-expression");
    }

    if (i >= vs->count || i < 0) {
        return newE(ERR_OUT_OF_BOUND, "lval_pop invoked with out of bound index");
    }

     lval* v = lval_pop(vs, i);

     lval_del(vs);

     return v;


}

lval* lval_eval_sexpr(lval* v);

lval* lval_eval(lval* v) {
    if (v->type == SE) { return lval_eval_sexpr(v); }

    return v;
}

lval* builtin_op(lval* vs, lval* sym) {
    if (vs->type != SE) {
        return newE(ERR_NOT_SEXPR, "Operation over not the S-expression");
    }
    if (vs->count < 1) {
        return newE(ERR_NOT_SEXPR, "Unsupported number of arguments the an S-expression");
    }
    if (sym->type != SY) {
        return newE(ERR_NOT_SEXPR, "Not a symboal as first argument in the S-expression");
    }
    for (int i = 0; i < vs->count; i++) {
        if (vs->cell[i]->type != I && vs->cell[i]->type != F) {
            lval_del(vs);
            lval_del(sym);
            return newE(ERR_NOT_SEXPR, "NaN operand in the S-expression");
        }
    }

    lval* a = lval_pop(vs, 0);

    if (sym->v.sym == MINUS && vs->count == 0) {
        switch (a->type) {
            case I: a->v.in = -a->v.in; break;
            case F: a->v.fn = -a->v.fn; break;
            default:
                lval_del(vs);
                lval_del(a);
                lval_del(sym);
                return newE(ERR_NOT_SEXPR, "NaN operand in the S-expression");
                break;
        }
    }

    while (vs->count > 0) {
        lval* b = lval_pop(vs, 0);

        switch (a->type) {
            case I:
                switch (b->type) {
                    case I:
                        switch (sym->v.sym) {
                            case PLUS:  a->v.in += b->v.in; break;
                            case MINUS: a->v.in -= b->v.in; break;
                            case MUL:   a->v.in *= b->v.in; break;
                            case DIV:
                                if (b->v.in == 0) {
                                    lval_del(a); lval_del(b); lval_del(sym);
                                    lval_del(vs);
                                    return newE(ERR_DIV_ZERO,"");
                                }
                                a->v.in /= b->v.in;
                                break;
                        }
                        break;
                    case F:
                        printf("I'm here! 1 ");
                        a->type = F;
                        a->v.fn = a->v.in;
                        switch (sym->v.sym) {
                            case PLUS:  a->v.fn += b->v.fn; break;
                            case MINUS: a->v.fn -= b->v.fn; break;
                            case MUL:   a->v.fn *= b->v.fn; break;
                            case DIV:   a->v.fn /= b->v.fn; break;
                                if (b->v.fn == 0) {
                                    lval_del(a); lval_del(b); lval_del(sym);
                                    lval_del(vs);
                                    return newE(ERR_DIV_ZERO,"");
                                }
                                a->v.fn /= b->v.fn;
                                break;
                        }
                        break;
                    default:
                        lval_del(vs);
                        lval_del(a);
                        lval_del(b);
                        lval_del(sym);
                        return newE(ERR_NOT_SEXPR, "NaN operand in the S-expression");
                        break;
                }
                break;
            case F:
                switch (b->type) {
                    case I:
                        b->type = F;
                        b->v.fn = b->v.in;
                        switch (sym->v.sym) {
                            case PLUS:  a->v.fn += b->v.fn; break;
                            case MINUS: a->v.fn -= b->v.fn; break;
                            case MUL:   a->v.fn *= b->v.fn; break;
                            case DIV:   a->v.fn /= b->v.fn; break;
                                if (b->v.fn == 0) {
                                    lval_del(a); lval_del(b); lval_del(sym);
                                    lval_del(vs);
                                    return newE(ERR_DIV_ZERO,"");
                                }
                                a->v.fn /= b->v.fn;
                                break;
                        }
                        break;
                    case F:
                        switch (sym->v.sym) {
                            case PLUS:  a->v.fn += b->v.fn; break;
                            case MINUS: a->v.fn -= b->v.fn; break;
                            case MUL:   a->v.fn *= b->v.fn; break;
                            case DIV:   a->v.fn /= b->v.fn; break;
                                if (b->v.fn == 0) {
                                    lval_del(a); lval_del(b); lval_del(sym);
                                    lval_del(vs);
                                    return newE(ERR_DIV_ZERO,"");
                                }
                                a->v.fn /= b->v.fn;
                                break;
                        }
                        break;
                    default:
                        lval_del(vs);
                        lval_del(a);
                        lval_del(b);
                        lval_del(sym);
                        return newE(ERR_NOT_SEXPR, "NaN operand in the S-expression");
                        break;

                }
                break;
            default:
                lval_del(vs);
                lval_del(a);
                lval_del(b);
                lval_del(sym);
                return newE(ERR_NOT_SEXPR, "NaN operand in the S-expression");
                break;
        }
        lval_del(b);
    }

    lval_del(vs);
    return a;



}

lval* lval_eval_sexpr(lval* v) {
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == E) {
            return lval_take(v, i);
        }
    }

    if (v->count == 0) { return v; }

    if (v->count == 1) { return lval_take(v, 0); }

    lval* sym = lval_pop(v, 0);
    if (sym->type != SY) {
        lval_del(sym);
        lval_del(v);
        return newE(ERR_NOT_SEXPR, "S-expression Does not start with symbol.");
    }

    lval* result = builtin_op(v, sym);
    return result;
}

int main(int argc, char** argv) {

    // Create parsers
    mpc_parser_t* Integer = mpc_new("integer");
    mpc_parser_t* Decimal = mpc_new("decimal");
    mpc_parser_t* Number  = mpc_new("number");
    mpc_parser_t* Symbol  = mpc_new("symbol");
    mpc_parser_t* Expr    = mpc_new("sexpr");
    mpc_parser_t* Sexpr   = mpc_new("expr");
    mpc_parser_t* Program = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                           \
                integer     : /-?[0-9]+/ ;                              \
                decimal     : /-?[0-9]+\\.[0-9]+/ ;                     \
                number      : <decimal> | <integer> ;                   \
                symbol      : '+' | '-' | '*' | '/' ;                   \
                sexpr       : '(' <expr>* ')' ;   \
                expr        : <number> | <symbol> | <sexpr> ;   \
                program     : /^/ <expr>* /$/ ;              \
            ",
            Integer, Decimal, Number, Symbol, Sexpr, Expr, Program);



    // Print Version and Exit Informaton
    puts("lis v0.4.0");
    puts("Press ^C to Exit\n");

    while(1) {
        mpc_result_t r;

        char* input = readline("> ");
        // Attempt to Parse the user Input
        if (mpc_parse("<stdin>", input, Program, &r)) {
            //mpc_ast_print(r.output);
            //lval_println(eval(r.output));
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        add_history(input);
        free (input);
    }

    mpc_cleanup(7, Number, Decimal, Integer, Symbol, Sexpr, Expr, Program);

    return 0;
}
