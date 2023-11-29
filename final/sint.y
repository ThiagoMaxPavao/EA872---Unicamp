%define api.pure full
%param { yyscan_t scanner }
%parse-param {p_no_command *comandos}

%code top {
    #include <stdlib.h> // para alocacao dinamica de memoria
    #include <stdio.h>
    #include "util.h"
}
%code requires {
    typedef void* yyscan_t;
}
%code {
  int yylex(YYSTYPE* yylvalp, yyscan_t scanner);
  void yyerror(yyscan_t unused, p_no_command *comandos, const char* msg);
}

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
