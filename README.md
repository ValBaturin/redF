# ProjectF
Functional Language

# Build
`lex grammar.l && bison -d grammar.y && gcc --std=c11 grammar.tab.c lex.yy.c fi.c`
