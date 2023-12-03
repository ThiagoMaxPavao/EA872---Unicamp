# Comandos de compilação para o servidor

# Cria tabela de definições de tokens para o flex e o analisador sintático sint.tab.c
bison -o sint.tab.c --defines=sint.tab.h sint.y

# Cria o analisador léxico lex.yy.c
flex -o lex.yy.c --header-file=lex.yy.h lex.l

# Combina os arquivos contendo os códigos dos analisadores, utilitários e principal do servidor
# em um executável único: server. Flags para compilação:
# -lfl      -> para o flex
# -ly       -> para o bison/yacc
# -lcrypt   -> para a chamada de sistema de criptografia crypt
# -lpthread -> para o uso de chamadas de sistema relativas à threads
gcc get.c util.c lex.yy.c sint.tab.c server.c base64.c -o server -lfl -ly -lcrypt -lpthread
