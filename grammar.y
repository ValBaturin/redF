%{
#include "fi.h"
#include "grammar.tab.h"
#include <unistd.h>

void yyerror (char *s);
int yylex();

%}

%union {
    double val; int huy; Node* node;
}

%token number
%token lbracket
%token rbracket

%type <val> number
%type <node> list
%type <node> entity
%type <node> entities
%type <node> program
%type <node> lbracket

%%

program: entity
|        entity program;
entity: number {$$ = newRNode($1);}
|        list
entities: entities entity   {addNode(yycurrent, $2);}
|         entity            {addNode(yycurrent, $1);}
list: lbracket entities rbracket { 
    print(yycurrent);
    $$ = yycurrent;
    yycurrent = newLNode();
    }

%%
