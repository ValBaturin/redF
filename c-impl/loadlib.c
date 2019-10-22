#include "loadlib.h"

int loadlib(lenv* env, char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        return 1;
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        yy_scan_string(line);
        yycurrent = newLNode();
        yyprogram = newLNode();
        yyparse();

        lval* x = lval_eval(env, lval_read(yyprogram));
        lval_del(x);
    }

    fclose(fp);
    if (line) {
        free(line);
    }
    return 0;
}
