typedef struct no_option {
    char* option;
    struct no_option* prox;
} no_option;
typedef no_option* p_no_option; // struct da lista ligada de opcoes/parametros

typedef struct no_command {
    char* command;
    p_no_option options;
    struct no_command* prox;
} no_command;
typedef no_command* p_no_command; // struct da lista ligada de comandos
