#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc/mpc.h"


// Evaluation result type. l is for lis.
// I - Integer
// F - Float
// E - Error
enum ltype{I, F, E};

// Error type
enum etype {
    ERR_DIV_ZERO,
    ERR_BAD_OP,
    ERR_BAD_NUM,
    ERR_UNK,
    ERR_NO_ERR,
};

typedef struct {
    enum ltype type;
    union {
        long int in;
        double fn;
        enum etype err;
    } v;
} lval;

lval new_lval(enum ltype type) {
    lval v;
    v.type = type;
    switch (type) {
        case I: v.v.in=0; break;
        case F: v.v.fn=0.; break;
        case E: v.v.err=ERR_NO_ERR; break;
    };

    return v;
}


void lval_print(lval v) {
    switch (v.type) {
        case I: printf("%li", v.v.in); break;
        case F: printf("%lf", v.v.fn); break;
        case E:
            if (v.v.err == ERR_DIV_ZERO) { printf("Error: Division By Zero."); }
            if (v.v.err == ERR_BAD_OP)   { printf("Error: Invalid Operator."); }
            if (v.v.err == ERR_BAD_NUM)  { printf("Error: Invalid Number."); }
            if (v.v.err == ERR_UNK)  { printf("Error: Unknown Error."); }
            if (v.v.err == ERR_NO_ERR)   { printf("Error: Error not set,\
                    possibly everything is ok."); }
            break;
    }
}


void lval_println(lval v) { lval_print(v); putchar('\n'); }


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


lval eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        if (strstr(t->tag, "decimal")) {
            errno = 0;
            lval v = new_lval(F);
            v.v.fn = strtod(t->contents, NULL);
            if (errno != ERANGE) { return v; }
            else { v.type = E; v.v.err = ERR_BAD_NUM; return v; }
        } else if (strstr(t->tag, "integer")) {
            errno = 0;
            lval v = new_lval(I);
            v.v.in = strtol(t->contents, NULL, 10);
            if (errno != ERANGE) { return v; }
            else { v.type = E; v.v.err = ERR_BAD_NUM; return v; }
        }
    }
    char* op = t->children[1]->contents;

    // The operator is always (hopefully) second child
    lval a = eval(t->children[2]);

    for (int i=3; strstr(t->children[i]->tag, "expr"); i++) {
        a = evalop(a, op, eval(t->children[i]));
    }

    return a;
}

int main(int argc, char** argv) {

    // Create parsers
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Decimal   = mpc_new("decimal");
    mpc_parser_t* Integer   = mpc_new("integer");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Program   = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                           \
                integer     : /-?[0-9]+/ ;                              \
                decimal     : /-?[0-9]+\\.[0-9]+/ ;                     \
                number      : <decimal> | <integer> ;                   \
                operator    : '+' | '-' | '*' | '/' ;                   \
                expr        : <number> | '(' <operator> <expr>+ ')' ;   \
                program     : /^/ <operator> <expr>+ /$/ ;              \
            ",
            Integer, Decimal, Number, Operator, Expr, Program);



    // Print Version and Exit Information
    puts("lis v0.3.0");
    puts("Press ^C to Exit\n");

    while(1) {
        mpc_result_t r;

        char* input = readline("> ");
        // Attempt to Parse the use Input
        if (mpc_parse("<stdin>", input, Program, &r)) {
            //mpc_ast_print(r.output);
            lval_println(eval(r.output));
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        add_history(input);
        free (input);
    }

    mpc_cleanup(6, Number, Decimal, Integer, Operator, Expr, Program);

    return 0;
}
