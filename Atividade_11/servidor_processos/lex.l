%{
	#include <stdlib.h> // para alocacao dinamica de memoria
	#include "util.h"
	#include "sint.tab.h"
    
	int yylex(void);        // para evitar erro do compilador gcc 
	int yyerror(char* s);   // para evitar erro do compilador gcc
	char* allocAndCopy(char* str); // funcao para copiar string para espaco alocado dinamicamente

	int lineCounter = 0; // contador da linha para avisar onde nao existe comando
%}
%START parametros
metodo      GET|HEAD|OPTIONS|TRACE|POST|PUT|DELETE
inicio      [.][/]http[-]dump.*
recebida    Mensagem[ ]recebida[:]
%%
<parametros>[^,\t\n\r]* { // reconhece parametro, podendo conter espacos. Apenas funciona
                          //  neste modo apos reconhecer um separados de campos ':'
                            yylval.string = allocAndCopy(yytext);
                            return WORD;
                        }
^#.*            { return COMMENT; }
:               { BEGIN parametros; return FIELD_SEPARATOR; }
,               { return OPTION_SEPARATOR; }
^{metodo}       {
                    yylval.string = allocAndCopy(yytext);
                    return METODO; // metodo reconhecido
                }
[\n\r]          {
                    BEGIN 0; // volta ao modo normal de execucao a cada incio de nova linha
                    yylval.number = ++lineCounter; // returns number of current line
                    return NEWLINE;
                }
[^ \t\n\r:,]+   {
                    yylval.string = allocAndCopy(yytext);
                    return WORD; // comando ou parametro/opcao
                }
^{inicio}$      ;
^{recebida}$    ; // ignora mensagens geradas pelo http-dump
[ \t]           ; // ignora tabulacao e espaco em branco
%%
// aloca dinamicamente o espaco para armazenar uma copia da string str
// retorna o ponteiro para a string alocada.
char* allocAndCopy(char* str) {
	int len, i;
	char* new;

    for(; *str == ' '; str++); // pula espacos do comeco da string, se houverem
	for(len = 0; str[len]!=0; len++); // descobre tamanho da string

	new = malloc(len + 1); // aloca vetor de char do mesmo tamanho que a string
						   // (mais um para colocar o \0)

	for(i = 0; i<=len; i++) new[i] = str[i]; // copia a string
	
	return new;
}
