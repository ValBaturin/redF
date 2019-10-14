all: fi fc

fi: lex.yy.c grammar.tab.c
	cc -Wall -g lex.yy.c grammar.tab.c ast.c lenv.c eval.c builtin.c lval.c loadlib.c fi.c -o fi -ledit -largp

fc: lex.yy.c grammar.tab.c
	cc -g lex.yy.c grammar.tab.c ast.c lenv.c eval.c builtin.c lval.c fc.c -o fc 

lex.yy.c: grammar.tab.c grammar.l
	lex grammar.l

grammar.tab.c: grammar.y
	bison -d grammar.y

clean: 
	rm -rf lex.yy.c grammar.tab.c grammar.tab.h fi fc fc.dSYM fi.dSYM
