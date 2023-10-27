ex3: lex.yy.c ex3.tab.c
	gcc -o ex3 lex.yy.c ex3.tab.c -lfl -ly

ex3.tab.c ex3.tab.h: ex3.y
	bison -d ex3.y

lex.yy.c: ex3.l ex3.tab.h
	flex ex3.l

clean:
	rm -f ex3 lex.yy.c ex3.tab.c ex3.tab.h
