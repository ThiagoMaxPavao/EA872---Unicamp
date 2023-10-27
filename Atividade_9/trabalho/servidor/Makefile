server: get.c util.c lex.yy.c sint.tab.c server.c
	gcc get.c util.c lex.yy.c sint.tab.c server.c -o server -lfl -ly

lex.yy.c: lex.l sint.tab.h
	flex lex.l

sint.tab.c sint.tab.h: sint.y
	bison -d sint.y

clean:
	rm -rf lex.yy.c sint.tab.c sint.tab.h server
