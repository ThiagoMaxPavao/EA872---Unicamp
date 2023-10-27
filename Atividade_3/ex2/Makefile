ex2: lex.yy.c ex2.tab.c
	gcc -o ex2 lex.yy.c ex2.tab.c -lfl -ly

ex2.tab.c ex2.tab.h: ex2.y
	bison -d ex2.y

lex.yy.c: ex2.l ex2.tab.h
	flex ex2.l

clean:
	rm -f ex2 lex.yy.c ex2.tab.c ex2.tab.h
