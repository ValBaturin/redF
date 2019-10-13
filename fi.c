#include "fi.h"
#include "grammar.tab.h"
#include <editline/readline.h>

void yyerror(char *s)
{
  fprintf(stderr, "%s\n", s);
}


void addNode(ast_node* dad, ast_node* child) {
    dad->value.children.size += 1;
    dad->value.children.nodes = realloc(dad->value.children.nodes, sizeof(ast_node*) * (dad->value.children.size));
    dad->value.children.nodes[(dad->value.children.size - 1)] = child;
    //printf("%p <- %p on %d\n", dad, 
    //        dad->value.children.nodes[(dad->value.children.size - 1)],
    //        (dad->value.children.size - 1));
}

ast_node* newRNode(double v) {
    ast_node* node = malloc(sizeof(ast_node));
    node->type = AST_REAL;
    node->value.r = v;
    printf("RNode %p\n", node);
    return node;
}

ast_node* newINode(int v) {
    ast_node* node = malloc(sizeof(ast_node));
    node->type = AST_INT;
    node->value.i = v;
    printf("INode %p\n", node);
    return node;
}

ast_node* newBNode(bool v) {
    ast_node* node = malloc(sizeof(ast_node));
    node->type = AST_BOOL;
    node->value.b = v;
    printf("BNode %p\n", node);
    return node;
}

ast_node* newNNode() {
    ast_node* node = malloc(sizeof(ast_node));
    node->type = AST_NULL;
    printf("NNode %p\n", node);
    return node;
}

ast_node* newANode(char* s) {
    ast_node* node = malloc(sizeof(ast_node));
    node->type = AST_ATOM;
    node->value.a = strdup(s);
    printf("ANode %p\n with value: %s\n", node, s);
    return node;
}

ast_node* newLNode() {
    ast_node* node = malloc(sizeof(ast_node));
    node->type = AST_LIST;
    node->value.children.size = 0;
    node->value.children.nodes = malloc(sizeof(ast_node*));
    printf("LNode %p\n", node);
    return node;
}

void print(ast_node* n) {
    printf("printing %p\n", n);
    printf("type is %d\n", n->type);
    if (n->type == AST_LIST) {
        for (int i = 0; i < n->value.children.size; ++i) {
            print(n->value.children.nodes[i]);
        }
    }
}

ast_node* yycurrent;

int main()
{
    while (1) {
    char* input = readline("> ");
    yy_scan_string(input);
    yycurrent = newLNode();
    yyparse();
    }
}
