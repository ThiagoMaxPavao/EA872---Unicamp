/* Enumeracao dos tipos de métodos HTTP possíveis */
typedef enum {
    GET,
    HEAD,
    OPTIONS,
    TRACE,
    POST,
    PUT,
    DELETE,
    INVALID
} Metodo;

/* Struct da lista ligada de opcoes/parametros */
typedef struct no_option {
    char* option;
    struct no_option* prox;
} no_option;
typedef no_option* p_no_option;

/* Struct da lista ligada de comandos */
typedef struct no_command {
    char* command;
    p_no_option options;
    struct no_command* prox;
} no_command;
typedef no_command* p_no_command;

/* Struct de input para a thread */
typedef struct {
    char *webspace;
    int socket_msg;
    int fd_log;
} thread_input_data;
typedef thread_input_data* thread_input_data_ptr;

/*
Aloca um comando na memoria, com command e options configurados pelos parametros
o campo proximo eh configurado com NULL e o ponteiro para o comando eh retornado
*/
p_no_command criaComando(char* command, p_no_option options);

/*
Adiciona um parametro a uma lista ligada ja existente.
o parametro se torna o novo primeiro elemento da lista ligada.
*/
p_no_option anexaParametro(p_no_option lista, char* option);

/*
Desaloca a memoria de uma lista ligada de comandos, liberando tambem suas listas de opcoes
*/
void liberaComandos(p_no_command lista);

/*
Desaloca a memoria de uma lista ligada de opcoes
*/
void liberaOpcoes(p_no_option lista);

/*
Imprime os comandos armazenados na lista ligada seguidos por seus parametros
*/
void imprimeComandos(p_no_command lista, int fd_resp, int fd_log);

/*
Imprime a linha de erro, quando encontra uma linha sem comando a esquerda do ':'
recebe o numero da linha atual, para indicar ao usuario onde ocorre o problema
a saida eh impressa na saida de erro padrao, stderr
*/
void imprimeErroSemComando(int lineNumber);

/*
Converte a string com a palavra de um método, por exemplo "GET", para
o valor na enumeração de métodos GET. retorna este valor.
Retorna INVALID (da enumeração) se a string não coincidir com nenhum dos métodos.
*/
Metodo stringParaMetodo(char* str);

/*
Imprime o cabecalho da requisição em dois arquivos, utilizando os seus descritores (fd_resp e fd_log).
Monta o cabecalho contendo:
O número de status e a seu texto equivalente, status = -1 omite este campo.
A data atual, formatada.
O tipo de conexão (keep_alive, closed, etc), copiando o parametro connection.
Se fd != -1: Há um arquivo sendo enviado então imprime as seguintes informações sobre ele
Data de última modificação
Tamanho em bytes
Tipo de conteúdo.
Se fd == -1, não há recurso junto à resposta então não há data de modificação mas é enviado
Content-Length: 0
Content-Type: text/html
*/
void cabecalho(int status, char* connection, char* filename, int fd, int fd_resp, int fd_log);

/*
Percorre o arquivo descrito por originFd e imprime todos os seus bytes em destinationFd.
Retorna -1 se ocorrer erro, 0 se tudo ocorrer bem
*/
int imprimeConteudo(int destinationFd, int originFd);

/*
dupPrintf = printf duplicado. Imprime o formato de string especificado em format
nos arquivos descritos por fd1 e fd2. Também pode receber os valores a serem inseridos,
logo após o formato estabelecido, assim como funciona printf.
*/
void dupPrintf(int fd1, int fd2, char const *format, ...);

/*
Retorna 1 se a string str termina com a string suffix e 0 caso contrário.
*/
int stringEndsWith(const char * str, const char * suffix);

/*
Verifica se o caminho do recurso acessa algum diretório fora (acima) do webspace.
Retorna 1 se o caminho for apenas interno ao webspace (confined).
E 0 se for um caminho invasivo.
*/
int isPathConfined(char resource[]);

/*
Percorre a lista ligada até encontrar um nó que tenha o comando Parameter.
Retorna um ponteiro para o valor de uma chave (comando)
Retorna NULL se não encontrar o parametro especificado
*/
char* getParameter(p_no_command comandos_local, char* parameter);
