%{
    #include <stdlib.h> // para alocacao dinamica de memoria
    #include <stdio.h>
    #include "nodes.h"

    int yylex(void);        // para evitar erro do compilador gcc
    int yyerror(char* s);   // para evitar erro do compilador gcc

    // funcoes utilitarias para as listas ligadas
    p_no_command criaComando(char* command, p_no_option options);
    p_no_option anexaParametro(p_no_option lista, char* option);
    void liberaComandos(p_no_command lista);
    void liberaOpcoes(p_no_option lista);
    void imprimeComandos(p_no_command lista);
    void imprimeErroSemComando(int lineNumber);

    p_no_command comandos = NULL; // inicializa lista de variaveis iniciadas
%}
%union {
    int number;
    char* string;
    p_no_option optionList;
    p_no_command commandPointer;
}
%token COMMENT
%token FIELD_SEPARATOR
%token OPTION_SEPARATOR
%token <number> NEWLINE
%token <string> WORD
%type <optionList> opcoes
%type <commandPointer> linha
%%
linhas: linha linhas    {   
                            if($1 != NULL){
                                $1->prox = comandos;
                                comandos = $1;
                            }
                        }
      | linha { comandos = $1; }
;
linha: COMMENT NEWLINE { $$ = NULL; }
     | WORD FIELD_SEPARATOR opcoes NEWLINE { $$ = criaComando($1, $3); }
     | WORD FIELD_SEPARATOR NEWLINE { $$ = criaComando($1, NULL); }
     | FIELD_SEPARATOR NEWLINE { imprimeErroSemComando($2); $$ = NULL; }
     | FIELD_SEPARATOR opcoes NEWLINE { imprimeErroSemComando($3); liberaOpcoes($2); $$ = NULL; }
;
opcoes: WORD OPTION_SEPARATOR opcoes { $$ = anexaParametro($3, $1); }
      | WORD { $$ = anexaParametro(NULL, $1); }
;
%%
void main() {
    yyparse();
    printf("\nFim Do Parsing, lista de comandos construída:\n\n");
    imprimeComandos(comandos);
    liberaComandos(comandos); // libera a memoria alocada
}

// Aloca um comando na memoria, com command e options configurados pelos parametros
// o campo proximo eh configurado com NULL e o ponteiro para o comando eh retornado
p_no_command criaComando(char* command, p_no_option options) {
    p_no_command novo = malloc(sizeof(no_command));
    novo->command = command; 
    novo->options = options;
    novo->prox = NULL;
    return novo;
}

// adiciona um parametro a uma lista ligada ja existente.
// o parametro se torna o novo primeiro elemento da lista ligada.
p_no_option anexaParametro(p_no_option lista, char* option) {
    p_no_option novo = malloc(sizeof(no_option));
    novo->option = option;
    novo->prox = lista;
    return novo;
}

// imprime os parametros armazenados em uma lista ligada de opcoes
// se a lista for vazia, avisa que nao ha parametros
// separa cada parametro com um espaco
void imprimeParametros(p_no_option lista) {
    if(lista == NULL) {
        printf("Sem parâmetros.\n");
        return;
    }
    
    printf("Parametros: ");
    while(lista != NULL) {
        printf("%s ", lista->option);
        lista = lista->prox;
    }
    printf("\n");
}

// imprime os comandos armazenados na lista ligada seguidos por seus parametros
void imprimeComandos(p_no_command lista) {
    while(lista != NULL) {
        printf("Comando: %s\n", lista->command);
        imprimeParametros(lista->options);
        printf("\n");
        lista = lista->prox;
    }
}

// imprime a linha de erro, quando encontra uma linha sem comando a esquerda do ':'
// recebe o numero da linha atual, para indicar ao usuario onde ocorre o problema
// a saida eh impressa na saida de erro padrao, stderr
void imprimeErroSemComando(int lineNumber) {
    fprintf(stderr, "Linha %d sem comando!\n", lineNumber);
}

// desaloca a memoria de uma lista ligada de opcoes
void liberaOpcoes(p_no_option lista) {
    while (lista != NULL) {
        p_no_option temp = lista;
        lista = lista->prox;
        free(temp->option); // Libera a string
        free(temp);         // Libera o no
    }
}

// desaloca a memoria de uma lista ligada de comandos, liberando tambem suas listas de opcoes
void liberaComandos(p_no_command lista) {
    while (lista != NULL) {
        p_no_command temp = lista;
        lista = lista->prox;
        free(temp->command);         // Libera a string
        liberaOpcoes(temp->options); // libera a lista de opcoes
        free(temp);                  // Libera o no
    }
}
