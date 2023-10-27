%{
#include <stdio.h>
int yylex(void);        // para evitar erro do compilador gcc 
int yyerror(char* s);   // para evitar erro do compilador gcc

float soma = 0;
%}
%union {
        float 	valor_real;
        int 	valor_inteiro;
       }
%token <valor_inteiro> INTEIRO 
%token <valor_real> REAL

%%
linhas:   linha
         | linhas linha
linha:	'\n'		{printf("=%f\n", soma); }
         | exp '\n'	{printf("=%f\n", soma); }
exp   : INTEIRO		{soma += $1; }
        | '+' INTEIRO	{soma += $2; }
        | '-' INTEIRO	{soma -= $2; }
        | '=' INTEIRO	{soma  = $2; }
        | '='		{soma  =  0; }
        | REAL		{soma += $1; }
        | '+' REAL	{soma += $2; }
        | '-' REAL	{soma -= $2; }
        | '=' REAL	{soma  = $2; }
%%
void main(){
    yyparse();
}
