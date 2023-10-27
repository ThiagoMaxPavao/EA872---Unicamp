all: ex6 http-dump

http-dump: http-dump.c
	gcc -o http-dump http-dump.c

ex6: lex.yy.c ex6.tab.c
	gcc -o ex6 lex.yy.c ex6.tab.c -lfl -ly

ex6.tab.c ex6.tab.h: ex6.y
	bison -d ex6.y

lex.yy.c: ex6.l ex6.tab.h
	flex ex6.l

clean:
	rm -f lex.yy.c ex6.tab.c ex6.tab.h
