%option reentrant bison-bridge

%{
    #include <stdlib.h> // para alocacao dinamica de memoria
    #include "util.h"
    #include "sint.tab.h"
    
    char* allocAndCopy(char* str); // funcao para copiar string para espaco alocado dinamicamente
%}
%START parametros
metodo      GET|HEAD|OPTIONS|TRACE|POST|PUT|DELETE
%%
<parametros>[^,\t\n\r]* { // reconhece parametro, podendo conter espacos. Apenas funciona
                          //  neste modo apos reconhecer um separados de campos ':'
                            yylval->string = allocAndCopy(yytext);
                            return WORD;
                        }
:               { BEGIN parametros; return FIELD_SEPARATOR; }
,               { return OPTION_SEPARATOR; }
^{metodo}       {
                    yylval->string = allocAndCopy(yytext);
                    return METODO; // metodo reconhecido
                }
[\r][\n]          {
                    BEGIN 0; // volta ao modo normal de execucao a cada incio de nova linha
                    return NEWLINE;
                }
[^ \t\n\r:,]+   {
                    yylval->string = allocAndCopy(yytext);
                    return WORD; // comando ou parametro/opcao
                }
[ \t]           ; // ignora tabulacao e espaco em branco
%%
