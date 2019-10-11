#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

int yyparse(void);
int yylex(void);
void yyerror(char *s);

typedef struct Node Node;

Node* newLNode();

struct Node
{
    int type;
    
    double value;
    int n;
    Node** nodes;
};

void addNode(Node* dad, Node* child);

Node* newRNode(double v);

Node* newBNode(bool v);

Node* newINode(int v);

Node* newNNode();

Node* newANode(char*);


Node* newNode();

void print(Node* n);

extern Node* yycurrent;

Node* yydrop_node(Node* n);
