ex4: lex.yy.c ex4.tab.c
	gcc -o ex4 lex.yy.c ex4.tab.c -lfl -ly

ex4.tab.c ex4.tab.h: ex4.y
	bison -d ex4.y

lex.yy.c: ex4.l ex4.tab.h
	flex ex4.l

clean:
	rm -f ex4 lex.yy.c ex4.tab.c ex4.tab.h
