%{
#include "fi.h"
#include "grammar.tab.h"
void yyerror (char *s);
int yylex();
%}
%%
\(                        {return lbracket;}
\)                        {return rbracket;}
"true"                    {yylval.b = true; return boolean;}
"false"                   {yylval.b = false; return boolean;}
"null"                    {return nulltoken;}
[+-]?([0-9])+(\.([0-9])+) {yylval.r = strtod(yytext, NULL); return real;}
[+-]?([0-9])+             {yylval.i = atoi(yytext); return integer;}
[a-zA-Z]+[a-zA-Z0-9]*     {yylval.s = strdup(yytext); return symbol;}
[-+*/\\]                  {yylval.s = strdup(yytext); return symbol;}
[ \t\n]                   ;
%%
int yywrap(void) {return 1;}
