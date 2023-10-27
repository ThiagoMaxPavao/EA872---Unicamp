%{
	#include <stdlib.h> // para alocacao dinamica de memoria
    #include <stdio.h>
    int yylex(void);        // para evitar erro do compilador gcc
    int yyerror(char* s);   // para evitar erro do compilador gcc

    typedef struct no {
        char* str;
        struct no* prox;
    } no;
    typedef no* p_no; // cria struct da lista ligada

    // funcoes auxiliares para utilizar a lista ligada
    p_no insere(p_no lista, char* str);
    int existe(p_no lista, char* str);
    int libera(p_no lista);

    p_no lista = NULL; // inicializa lista de variaveis iniciadas
%}
%union {
    char* string;
}
%token INICIO_MAIN
%token FIM_MAIN
%token TIPO
%token PONTO_VIRGULA
%token VIRGULA
%token <string> NOMEVAR
%token IGUAL
%token OPERA
%token PARENTESES_ESQ
%token PARENTESES_DIR
%token VALOR
%%
prog_fonte: INICIO_MAIN conteudo_prog FIM_MAIN
;
conteudo_prog: declaracoes expressoes
;
declaracoes: linha_declara declaracoes
           | linha_declara
;
linha_declara: TIPO variaveis PONTO_VIRGULA
;
variaveis: identificador VIRGULA variaveis
         | identificador
;
identificador: NOMEVAR { free($1); }
             | NOMEVAR IGUAL VALOR { lista = insere(lista, $1); }
;
expressoes: linha_executavel expressoes
          | linha_executavel
;
linha_executavel: NOMEVAR IGUAL operacoes PONTO_VIRGULA { lista = insere(lista, $1); }
;
operacoes: operacoes OPERA operacoes
         | PARENTESES_ESQ operacoes PARENTESES_DIR
         | VALOR
         | NOMEVAR  {
                        if(!existe(lista, $1))
                            fprintf(stderr, "Uso de variavel nao iniciada! Nome: %s\n", $1);
                        free($1);
                    }
;
%%
void main() {
    yyparse();
    libera(lista); // fim da execucao, libera a memoria
}

// cria um novo no para a lista, liga a lista atual a ele e o retorna
// este no deve ser utilizado como base da lista
p_no insere(p_no lista, char* str) {
    p_no novo = malloc(sizeof(no));
    novo->str = str;
    novo->prox = lista;
    return novo;
}

// retorna 1 se as strings forem iguais, 0 caso contrario
int comparaStrings(char* str1, char* str2) {
    for(int i = 0; str1[i] == str2[i]; i++)
        if(str1[i] == 0) return 1;
    return 0;
}

// percorre a lista ligada ate encontrar uma string igual a passada como parametro
// retorna 1 se encontrar, e 0 se percorrer a lista toda e nao encontrar
int existe(p_no lista, char* str) {
    while(lista != NULL) {
        if(comparaStrings(lista->str, str))
            return 1;
        lista = lista->prox;
    }
    return 0;
}

// libera a memoria. Desalocando as strings armazenadas nela e os nos.
int libera(p_no lista) {
    while (lista != NULL) {
        p_no temp = lista;
        lista = lista->prox;
        free(temp->str); // Libera a string
        free(temp);      // Libera o no
    }
}
