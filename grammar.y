%{
#include "ast.h"
#include "grammar.tab.h"
#include <unistd.h>

void yyerror (char *s);
int yylex();

%}

%union {
    double r; int i; bool b; char* s; ast_node* node;
}

%token integer
%token real
%token boolean
%token nulltoken
%token symbol
%token lbracket
%token rbracket
%token quote

%type <i> integer
%type <r> real
%type <b> boolean
%type <s> symbol
%type <node> list
%type <node> qlist
%type <node> entity
%type <node> entities
%type <node> program
%type <node> lbracket

%%

program: entity {addNode(yyprogram, $1);}
|        entity program {addNode(yyprogram, $1);}
entity: real      {$$ = newRNode($1);}
|       integer   {$$ = newINode($1);}
|       boolean   {$$ = newBNode($1);}
|       nulltoken {$$ = newNNode();}
|       symbol    {$$ = newANode($1);}
|       list
|       qlist
entities: entity entities   {addNode(yycurrent, $1);}
|         entity            {addNode(yycurrent, $1);}
list: lbracket entities rbracket {$$ = yycurrent;
                                  yycurrent = newLNode();}
|     lbracket rbracket          {$$ = yycurrent;
                                  yycurrent = newLNode();}
qlist: quote entity {$$ = newQNode();
                     addNode($$, $2);}
%%
