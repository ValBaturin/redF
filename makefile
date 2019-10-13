fi: lex.yy.c grammar.tab.c
	gcc -g lex.yy.c grammar.tab.c fi.c main.c -o fi -ledit

lex.yy.c: grammar.tab.c grammar.l
	lex grammar.l

grammar.tab.c: grammar.y
	bison -d grammar.y

clean: 
	rm -rf lex.yy.c grammar.tab.c grammar.tab.h fi fi.dSYM
