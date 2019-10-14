#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "lenv.h"
#include "eval.h"

#include "ast.h"
#include "grammar.tab.h"

int loadlib(lenv* env, char* filename);
