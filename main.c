#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc/mpc.h"


int fevalop(double* a, char* op, double b) {
    if (strcmp(op, "+") == 0) { *a += b; return 0; }
    if (strcmp(op, "-") == 0) { *a -= b; return 0; }
    if (strcmp(op, "*") == 0) { *a *= b; return 0; }
    if (strcmp(op, "/") == 0) { *a /= b; return 0; }
    return 1;
}


long ievalop(long int* a, char* op, long int b) {
    if (strcmp(op, "+") == 0) { *a += b; return 0; }
    if (strcmp(op, "-") == 0) { *a -= b; return 0; }
    if (strcmp(op, "*") == 0) { *a *= b; return 0; }
    if (strcmp(op, "/") == 0) { *a /= b; return 0; }
    return 1;
}

int eval(mpc_ast_t* t, long int* in, double* fn, int fpa, char* op) {
    // Floating point arithmetic?
    // Buffer integer
    long int bin = 0;
    // Buffer float
    double bfn = 0.;
    if (strstr(t->tag, "number")) {
        if (strstr(t->tag, "decimal")) {
            fpa = 1;
            //fevalop(fn, op, bfn);
            *fn = atof(t->contents);
            return fpa;
        } else if (strstr(t->tag, "integer")) {
            if (fpa) {
                *fn = atol(t->contents);

            } else {
                *in = atol(t->contents);
            }
            //ievalop(in, op, bin);
            return fpa;
        } else {
            // shouldn't fall here
            puts("shouldn't fall here");
        }
    } else {
        op = t->children[1]->contents;
    }

    // The operator is always (hopefully) second child
    fpa = eval(t->children[2], in, fn, fpa, op);

    for (int i=3; strstr(t->children[i]->tag, "expr"); i++) 
        // If we work on floating pointer numbers...
        if (fpa) {
            fpa = eval(t->children[i], &bin, &bfn, fpa, op);
            fevalop(fn, op, bfn);
        // And if we don't...
        } else {
            fpa = eval(t->children[i], &bin, &bfn, fpa, op);
            if (fpa) {
                // Don't forget to cast second argument to floating poing
                // if needed
                *fn = *in;
                fevalop(fn, op, bfn);
            } else {
                ievalop(in, op, bin);
            }
        }
    return fpa;
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
    puts("lis v0.2.0");
    puts("Press ^C to Exit\n");

    while(1) {
        mpc_result_t r;

        char* input = readline("> ");
        // Attempt to Parse the use Input
        if (mpc_parse("<stdin>", input, Program, &r)) {
            //mpc_ast_print(r.output);
            long int ires = 0;
            double dres = 0;
            int fpa = eval(r.output, &ires, &dres, 0, "");
            if (fpa) {
                printf("< %lf\n", dres);
            } else {
                printf("< %li\n", ires);
            }
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
