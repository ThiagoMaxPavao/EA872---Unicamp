%{
    #include <stdlib.h> // para alocacao dinamica de memoria
    #include <stdio.h>
    #include "util.h"

    int yylex(void);        // para evitar erro do compilador gcc
    int yyerror(p_no_command *comandos, char* s);   // para evitar erro do compilador gcc
%}
%parse-param {p_no_command *comandos}
%union {
    char* string;
    p_no_option optionList;
    p_no_command commandPointer;
}
%token COMMENT
%token FIELD_SEPARATOR
%token OPTION_SEPARATOR
%token NEWLINE
%token <string> WORD
%token <string> METODO
%type <optionList> opcoes
%type <optionList> opcoes-metodo
%type <commandPointer> linha
%%
linhas: linha linhas    {
                            if($1 != NULL){
                                $1->prox = *comandos;
                                *comandos = $1;
                            }
                        }
      | linha { *comandos = $1; }
;
linha: COMMENT NEWLINE { $$ = NULL; }
     | WORD FIELD_SEPARATOR opcoes NEWLINE { $$ = criaComando($1, $3); }
     | WORD FIELD_SEPARATOR NEWLINE { $$ = criaComando($1, NULL); }
     | METODO opcoes-metodo NEWLINE { $$ = criaComando($1, $2); }
     | NEWLINE { $$ = NULL; }
;
opcoes: WORD OPTION_SEPARATOR opcoes { $$ = anexaParametro($3, $1); }
      | WORD { $$ = anexaParametro(NULL, $1); }
;
opcoes-metodo: WORD opcoes-metodo { $$ = anexaParametro($2, $1); }
             | WORD { $$ = anexaParametro(NULL, $1); }
%%
