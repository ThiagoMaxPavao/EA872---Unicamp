ex5: lex.yy.c ex5.tab.c
	gcc -o ex5 lex.yy.c ex5.tab.c -lfl -ly

ex5.tab.c ex5.tab.h: ex5.y
	bison -d ex5.y

lex.yy.c: ex5.l ex5.tab.h
	flex ex5.l

clean:
	rm -f ex5 lex.yy.c ex5.tab.c ex5.tab.h
