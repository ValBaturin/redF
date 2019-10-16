#ifndef AST
#define AST

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int yyparse(void);
int yylex(void);
void yyerror(char *s);

typedef struct ast_node ast_node;

enum asttype {
    AST_REAL,
    AST_BOOL,
    AST_INT,
    AST_NULL,
    AST_ATOM,
    AST_LIST,
    AST_QLIST,
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

ast_node* newLNode();

ast_node* newQNode();

void print(ast_node* n);

extern ast_node* yycurrent;
extern ast_node* yyprogram;

ast_node* yydrop_node(ast_node* n);

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

#endif
