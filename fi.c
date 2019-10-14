#include "lval.h"
#include "lenv.h"
#include "builtin.h"
#include "eval.h"

#include "ast.h"
#include "grammar.tab.h"

#include "loadlib.h"

#include <editline/readline.h>
#include <argp.h>

const char *argp_program_version = "0.13";
const char *argp_program_bug_address = "<val.baturin@serokell.io> or <alexnryndin@gmail.com>";
static char doc[] = "redF interpreter";
static char args_doc[] = "";
static struct argp_option options[] = { 
    { "nostd", 'e', 0, 0, "do not include the base library"},
    { 0 } 
};

struct arguments {
    enum { STD, NO_STD } mode;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'e': arguments->mode = NO_STD; break;
    case ARGP_KEY_ARG: return 0;
    default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}
 
static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char** argv) {

    struct arguments arguments;

    arguments.mode = STD;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    // Print Version and Exit Informaton

    puts("lis v0.13.0");
    puts("Press ^C to Exit\n");

    lenv* env = new_lenv();
    lenv_add_builtins(env);

    if (arguments.mode == STD) {
        loadlib(env, "libraries/base.lisp");
    }

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
