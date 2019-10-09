#include "fi.h"
#include "grammar.tab.h"

void yyerror(char *s)
{
  fprintf(stderr, "%s\n", s);
}


void addNode(Node* dad, Node* child) {
    dad->n += 1;
    dad->nodes = malloc(sizeof(Node*) * (dad->n));
    dad->nodes[(dad->n - 1)] = child;
    printf("%p <- %p on %d\n", dad, dad->nodes[(dad->n - 1)], (dad->n - 1));
}

Node* newRNode(double v) {
    Node* node = malloc(sizeof(Node));
    node->type = 1;
    node->n = 0;
    node->value = v;
    printf("RNode %p\n", node);
    return node;
}

Node* newLNode() {
    Node* node = malloc(sizeof(Node));
    node->type = 0;
    node->n = 0;
    printf("LNode %p\n", node);
    return node;
}

void print(Node* n) {
    printf("printing %p\n", n);
    printf("type is %d\n", n->type);
    printf("it has %d children:\n", n->n);
    for (int i = 0; i < n->n; ++i) {
        printf("child %p\n", n->nodes[i]);
        print(n->nodes[i]);
    }
}

Node* yycurrent;

Node* yydrop_node(Node* n) {
    n->n = 0;
    return n;
}

int main()
{
    yycurrent = newLNode();
    yyparse();
}
