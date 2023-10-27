%{
    #include <stdlib.h> // para alocacao dinamica de memoria
    #include "ex4.tab.h"
    int yylex(void);        // para evitar erro do compilador gcc 
    int yyerror(char* s);   // para evitar erro do compilador gcc
    char* allocAndCopy(char* str); // funcao para copiar string para espaco alocado dinamicamente
%}
LET [a-zA-Z]
DIG [0-9]
%%
main[(][)][ ][{]            { return INICIO_MAIN; }
[}]                         { return FIM_MAIN; }
int|float                   { return TIPO; }
{LET}({LET}|{DIG})*         {
                                yylval.string = allocAndCopy(yytext);
                                return NOMEVAR;
                            }
;                           { return PONTO_VIRGULA; }
,                           { return VIRGULA; }
=                           { return IGUAL; }
[+]|[-]|[*]|[/]             { return OPERA; }
[(]                         { return PARENTESES_ESQ; }
[)]                         { return PARENTESES_DIR; }
({DIG}+)|({DIG}*[.]{DIG}+)  { return VALOR; }
[ \t\n]                     ; // ignora caracteres espaco, tabulacao e quebra de linha
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
