%{
	#include <stdlib.h> // para alocacao dinamica de memoria
	#include "nodes.h"
	#include "ex5.tab.h"
	int yylex(void);        // para evitar erro do compilador gcc 
	int yyerror(char* s);   // para evitar erro do compilador gcc
	char* allocAndCopy(char* str); // funcao para copiar string para espaco alocado dinamicamente

	int lineCounter = 0; // contador da linha para avisar onde nao existe comando
%}
%%
^#.*        { return COMMENT; }
:           { return FIELD_SEPARATOR; }
,           { return OPTION_SEPARATOR; }
[\n]        {
				yylval.number = ++lineCounter; // returns number of current line
				return NEWLINE;
			}
[^ \t\n:,]+ {
                yylval.string = allocAndCopy(yytext);
                return WORD; // comando ou parametro/opcao
            }
[ \t]       ; // ignora tabulacao e espaco em branco
%%
// aloca dinamicamente o espaco para armazenar uma copia da string str
// retorna o ponteiro para a string alocada.
char* allocAndCopy(char* str) {
	int len, i;
	char* new;

	for(len = 0; str[len]!=0; len++); // descobre tamanho da string

	new = malloc(len + 1); // aloca vetor de char do mesmo tamanho que a string
						   // (mais um para colocar o \0)

	for(i = 0; i<=len; i++) new[i] = str[i]; // copia a string
	
	return new;
}
