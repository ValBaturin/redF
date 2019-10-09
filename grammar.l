%{
#include "fi.h"
#include "grammar.tab.h"
void yyerror (char *s);
int yylex();
%}
%%
\(      {return lbracket;}
\)      {return rbracket;}
[+-]?([0-9])+(\.([0-9])+)? {yylval.val = strtod(yytext, NULL); return number;}
[ \t\n] ;
%%
int yywrap(void) {return 1;}
