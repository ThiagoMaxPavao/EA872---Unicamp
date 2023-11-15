server: get.c util.c lex.yy.c sint.tab.c server.c base64.c
	gcc get.c util.c lex.yy.c sint.tab.c server.c base64.c -o server -lfl -ly -lcrypt

lex.yy.c: lex.l sint.tab.h
	flex lex.l

sint.tab.c sint.tab.h: sint.y
	bison -d sint.y

clean:
	rm -rf lex.yy.c sint.tab.c sint.tab.h server
