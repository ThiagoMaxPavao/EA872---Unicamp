#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "util.h"
#include <string.h>
#include <netinet/in.h>
#include <sys/signal.h>

extern int get(char* webspace, char* resource, int* fd, char* filename); // funcao para acesso facil ao sistema de arquivos
extern p_no_command comandos; // lista de comandos
extern void yy_scan_string(const char* string); // funcao que passa string para o analisador lexico
extern int yyparse(void); // funcao que faz o parsing
int fd_log, socket_server;

int processRequest(char *webspace, int socket_msg, int fd_log) {
    char filename[50], *connectionState;
    char request[1024];
    int n_read;

    n_read = read(socket_msg, request, sizeof(request) - 1);
    if(n_read <= 0) return -1; // cancela se não leu nada
    request[n_read] = 0;

    printf("Requisição recebida:\n%s", request);

    /* Imprime Requisição feita na tela e no arquivo de log */
    dprintf(fd_log, "\n\n\n----- Request -----\n"); // formatacao do logfile
    dprintf(fd_log, "%s", request);
    dprintf(fd_log, "----- Response Header -----\n"); // formatacao do logfile

    yy_scan_string(request);
    yyparse(); // realiza o parsing, montando a lista ligada

    /* 
        Percorre a lista ligada até encontrar um nó que tenha o comando Connection.
        Salva o valor de seu parametro para enviar na resposta posteriormente, mantendo
        a mesma definida pelo cliente em sua requisição.
    */
    for(p_no_command comando = comandos; comando != NULL; comando = comando->prox) {
        if(strcmp(comando->command, "Connection") == 0) {
            connectionState = comando->options->option;
            break;
        }
    }

    /* Trata cada caso de método, incluindo o caso de bad request e not implemented */
    int fd, status;
    switch(stringParaMetodo(comandos->command)) {
        case GET:
        status = get(webspace, comandos->options->option, &fd, filename);
        cabecalho(status, connectionState, filename, fd, socket_msg, fd_log);
        imprimeConteudo(socket_msg, fd);
        if(fd != -1) close(fd);
        break;

        case HEAD:
        status = get(webspace, comandos->options->option, &fd, filename);
        cabecalho(status, connectionState, filename, fd, socket_msg, fd_log);
        if(fd != -1) close(fd);
        break;

        case OPTIONS:
        dupPrintf(socket_msg, fd_log, "HTTP/1.1 200 OK\r\n");
        dupPrintf(socket_msg, fd_log, "Allow: GET, HEAD, OPTIONS, TRACE\r\n");
        cabecalho(-1, connectionState, filename, -1, socket_msg, fd_log);
        break;

        case TRACE:
        dupPrintf(socket_msg, fd_log, "HTTP/1.1 200 OK\r\n");
        imprimeComandos(comandos->prox, socket_msg, fd_log);
        break;

        case INVALID:
        cabecalho(400, "close", filename, -1, socket_msg, fd_log); // Bad Request
        break;

        default:
        cabecalho(501, "close", filename, -1, socket_msg, fd_log); // Not Implemented
        break;
    }

    dprintf(fd_log, "----- End -----\n"); // formatacao do logfile

    /* libera toda a memoria alocada para as listas ligadas e as strings. */
    liberaComandos(comandos);
    comandos = NULL;
}

void closeServer() {
    printf("\nDesligando servidor e saindo...\n");

    close(socket_server); // Fecha servidor
    close(fd_log); // Fecha arquivo de log
    exit(0);
}

int main(int argc, char *argv[]) {
    int socket_msg;
    struct sockaddr_in server, client;
    int addr_size = sizeof(client);
    
    /* Verifica formato da chamada */
    if(argc != 4) {
        printf("Uso: %s <Web Space> <Porta> <Arquivo de Log>\n", argv[0]);
        exit(1);
    }

    /* Abre os arquivos, indicando se houver erro */
    if((fd_log = open(argv[3], O_APPEND | O_CREAT | O_WRONLY, 0600)) == -1) {
        perror("Erro na abertura do arquivo de Log");
        exit(2);
    }

    if((socket_server = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erro na criação do socket");
        exit(3);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = INADDR_ANY;

    if(bind(socket_server, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("Erro em bind");
        exit(4);
    }

    if(listen(socket_server, 5) == -1) {
        perror("Erro em listen");
        exit(5);
    }

    /*
        ignora SIGPIPE para não parar execução 
        quando cliente fecha conexão antes dos dados 
        terminarem de serem enviados.
    */
    signal(SIGPIPE, SIG_IGN);
    
    /*
        Captura sinal de interrupção de execução e manda
        para closeServer, que fecha os arquivos e então encerra a execulçao
    */
    signal(SIGINT, closeServer);

    while(1) {
        socket_msg = accept(socket_server, (struct sockaddr*)&client, &addr_size);
        processRequest(argv[1], socket_msg, fd_log);
        close(socket_msg);
    }
}
