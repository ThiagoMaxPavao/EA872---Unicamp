prog: lex.yy.c
	gcc -o prog lex.yy.c -lfl

lex.yy.c: prog.l
	flex prog.l
