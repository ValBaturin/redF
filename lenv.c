#include "lenv.h"
#include "err.h"

lenv* new_lenv() {
    lenv* env = malloc(sizeof(lenv));
    env->par = NULL;
    env->count = 0;
    env->syms = NULL;
    env->vals = NULL;
    return env;
}

void lenv_del(lenv* env) {
    for (int i = 0; i < env->count; i++) {
        free(env->syms[i]);
        lval_del(env->vals[i]);
    }

    free(env->syms);
    free(env->vals);
    free(env);

    return;
}

lenv* lenv_copy(lenv* env) {
    lenv* nenv = malloc(sizeof(lenv));
    nenv->par = env->par;
    nenv->count = env->count;
    nenv->syms = malloc(sizeof(char*) * nenv->count);
    nenv->vals = malloc(sizeof(lval*) * nenv->count);

    for (int i = 0; i < env->count; i++) {
        nenv->syms[i] = malloc(strlen(env->syms[i]) + 1);
        strcpy(nenv->syms[i], env->syms[i]);
        nenv->vals[i] = lval_copy(env->vals[i]);
    }

    return nenv;
}

lval* lenv_get(lenv* env, lval* var) {
    // var must be a symbol
    LASSERT_SAFE(var->type == SY, ERR_BAD_OP,
            "Not a variable name");

    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->syms[i], var->sym) == 0) {
            // TODO: why copy?
            return lval_copy(env->vals[i]);
        }
    }

    if (env->par) {
        return lenv_get(env->par, var);
    } else {
        return newE(ERR_NOT_DEFINED, "Variable %s is not defined", var->sym);
    }

}

// Put var in local (innermost, passed) environment
void lenv_put(lenv* env, lval* var, lval* v) {

    for (int i = 0; i < env->count; i++) {
    // If we found var
        if (strcmp(env->syms[i], var->sym) == 0) {
            lval_del(env->vals[i]);
            env->vals[i] = lval_copy(v);
            return;
        }
    }
    // If we didn't
    env->count++;
    env->vals = realloc(env->vals, sizeof(lval*) * env->count);
    env->syms = realloc(env->syms, sizeof(char*) * env->count);
    env->vals[env->count-1] = lval_copy(v);
    env->syms[env->count-1] = malloc(strlen(var->sym)+1);
    strcpy(env->syms[env->count-1], var->sym);

    return;
}

// Put var in global environment
void lenv_def(lenv* env, lval* var, lval* v) {
    while (env->par) {
        env = env->par;
    }

    lenv_put(env, var, v);
    return;
}

void env_print(lenv* env) {
    puts("env_print");
    for (int i = 0; i < env->count; i++) {
        printf("sym - %s, lval - %s\n", env->syms[i], ltype_name(env->vals[i]->type));
    }
    if (env->par) {
        puts("Have par");
        env_print(env->par);
    }
    return;
}
