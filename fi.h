#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int yyparse(void);
int yylex(void);
void yyerror(char *s);

typedef struct ast_node ast_node;

ast_node* newLNode();

enum asttype {
    AST_REAL,
    AST_BOOL,
    AST_INT,
    AST_NULL,
    AST_ATOM,
    AST_LIST,
};

struct ast_node
{
    enum asttype type;
    
    union {
        double r;
        bool b;
        int i;
        char* a;
        struct {
            int size;
            ast_node** nodes;
        } children;
    } value;
};

void addNode(ast_node* dad, ast_node* child);

ast_node* newRNode(double v);

ast_node* newBNode(bool v);

ast_node* newINode(int v);

ast_node* newNNode();

ast_node* newANode(char*);

ast_node* newNode();

void print(ast_node* n);

extern ast_node* yycurrent;

ast_node* yydrop_node(ast_node* n);
