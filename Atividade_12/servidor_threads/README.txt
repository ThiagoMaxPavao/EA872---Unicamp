Para fazer a compilação e gerar o arquivo executável 'server' basta executar, na ordem dada, os comandos

bison -d sint.y
flex lex.l
gcc get.c util.c lex.yy.c sint.tab.c server.c -o server -lfl -ly

Neste último pode ser necessário remover as flags de bibliotecas '-lfl' e/ou '-ly' a depender do estado do sistema.
Alternativamente, dentre os arquivos existe um 'Makefile' que pode ser utilizado para gerar o executável. Neste caso basta executar

make
