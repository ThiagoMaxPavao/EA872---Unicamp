server: get.c util.c lex.yy.c sint.tab.c server.c base64.c lex.yy.h
	gcc get.c util.c lex.yy.c sint.tab.c server.c base64.c -o server -lfl -ly -lcrypt -lpthread

lex.yy.c lex.yy.h: lex.l sint.tab.h
	flex -o lex.yy.c --header-file=lex.yy.h lex.l

sint.tab.c sint.tab.h: sint.y
	bison -o sint.tab.c --defines=sint.tab.h sint.y

clean:
	rm -rf lex.yy.c lex.yy.h sint.tab.c sint.tab.h server
