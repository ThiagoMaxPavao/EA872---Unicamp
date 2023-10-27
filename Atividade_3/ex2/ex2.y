%token INTEIRO

%{
	#include <stdio.h>
%}

%%
linhas: linhas linha {printf("Sintático: linha vale (produção composta) = %d\n", $2); }
	  | linha {printf("Sintático: linha vale (produção simples) = %d\n", $1); }
;
linha: INTEIRO '\n' {
						printf("Sintático: INTEIRO vale = %d\n", $1); 
						$$ = $1;
					}
;
%%

void main()	{
	yyparse();
}

void yyerror (char const *s)
{
	fprintf (stderr, "%s\n", s);
}
