%{
#include "ex3.tab.h"
int yylex(void);        // para evitar erro do compilador gcc 
int yyerror(char* s);   // para evitar erro do compilador gcc
%}
digito    [0-9]+
hexa      0x[0-9a-fA-F]+
frac      [0-9]*\.[0-9]*
operador  [-+=]
nl        \n
%%
{digito}	{
		  sscanf(yytext, "%d", &yylval.valor_inteiro);
		  return INTEIRO;
		}
{hexa}	{
		  sscanf(yytext, "%x", &yylval.valor_inteiro);
		  return INTEIRO;
		} 
{frac}          {
		   sscanf(yytext, "%f", &yylval.valor_real);
		  return REAL;
		}
{operador}	return yytext[0];
{nl}		return ('\n');
