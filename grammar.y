%{
#include "fi.h"
#include "grammar.tab.h"
#include <unistd.h>

void yyerror (char *s);
int yylex();

%}

%union {
    double r; int i; bool b; Node* node;
}

%token integer
%token real
%token boolean
%token lbracket
%token rbracket
%token nulltoken

%type <i> integer
%type <r> real
%type <b> boolean
%type <node> list
%type <node> entity
%type <node> entities
%type <node> program
%type <node> lbracket

%%

program: entity {yycurrent = $1; print(yycurrent); // remove last yycurrent node
                }
|        entity program;
entity: real      {$$ = newRNode($1);}
|       integer   {$$ = newINode($1);}
|       boolean   {$$ = newBNode($1);}
|       nulltoken {$$ = newNNode();}
|       list
entities: entity entities   {addNode(yycurrent, $1);}
|         entity            {addNode(yycurrent, $1);}
list: lbracket entities rbracket { 
    $$ = yycurrent;
    yycurrent = newLNode();
    }

%%
