%{
	#include "ex2.tab.h"	// Este include file é gerado pelo bison. Por isso, 
				// compile o analisador sintático antes do léxico.
	int yylex(void);        // para evitar erro do compilador gcc 
	int yyerror(char* s);   // para evitar erro do compilador gcc
%}
%%
[0-9a-fA-F]+	{
		sscanf(yytext, "%x", &yylval);
		printf("Léxico: yytext (texto) = %s \t yyval (valor) =  %d\n", yytext, yylval);
		return (INTEIRO);
	}
\n	return ('\n');
%%
