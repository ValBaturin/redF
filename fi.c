#include "lval.h"
#include "lenv.h"
#include "builtin.h"
#include "eval.h"

#include "ast.h"
#include "grammar.tab.h"

#include <editline/readline.h>

int main(int argc, char** argv) {

    // Print Version and Exit Informaton
    puts("lis v0.12.0");
    puts("Press ^C to Exit\n");

    lenv* env = new_lenv();
    lenv_add_builtins(env);

    while(1) {
        char* input = readline("> ");
        yy_scan_string(input);
        yycurrent = newLNode();
        yyprogram = newLNode();
        yyparse();

        lval* x = lval_eval(env, lval_read(yyprogram));
        lval_println(x);
        lval_del(x);

        add_history(input);
        free(input);

    }

    return 0;
}
