#include "lval.h"
#include "lenv.h"
#include "builtin.h"
#include "eval.h"

#include "ast.h"
#include "grammar.tab.h"

int main(int argc, char** argv) {

    lenv* env = new_lenv();
    lenv_add_builtins(env);

    if (argc <  2) {
        fprintf(stderr, "Usage: ./fc filename\n");
        exit(1);
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "%s doesn't exist\n", argv[1]);
        exit(1);
    }
    char* input;

    fseek( fp , 0L , SEEK_END);
    long lSize = ftell( fp );
    rewind( fp );
    
    /* allocate memory for entire content */
    input = calloc( 1, lSize+1 );
    if(!input) {
        fclose(fp),fputs("memory alloc fails",stderr),exit(1); 
    }
    
    /* copy the file into the buffer */
    if( 1 != fread( input , lSize, 1 , fp) ) {
        fclose(fp),free(input),fputs("entire read fails",stderr),exit(1); }

    yy_scan_string(input);
    yycurrent = newLNode();
    yyprogram = newLNode();
    yyparse();

    lval* x = lval_eval(env, lval_read(yyprogram));
    lval_println(x);

    lval_del(x);

    fclose(fp);
    free(input);

    return 0;
}
