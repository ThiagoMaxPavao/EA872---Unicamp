#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "util.h"
#include <string.h>

extern int get(char* webspace, char* resource, int* fd, char* filename); // funcao para acesso facil ao sistema de arquivos
extern p_no_command comandos; // lista de comandos
extern FILE *yyin; // arquivo de entrada do analisador lexico
extern int yyparse(void); // funcao que faz o parsing

int main(int argc, char *argv[]) {
    char filename[50], *connectionState;
    int fd_resp, fd_log;

    /* Verifica formato da chamada */
    if(argc != 5) {
        printf("Uso: %s <Web Space> <Requisicao> <Resposta> <Arquivo de Log>\n", argv[0]);
        exit(1);
    }

    /* Abre os arquivos, indicando se houver erro */
    if((fd_resp = open(argv[3], O_CREAT | O_WRONLY, 0600)) == -1) {
        printf("Erro na abertura do arquivo de Resposta.\n");
        exit(3);
    }

    if((fd_log = open(argv[4], O_APPEND | O_CREAT | O_WRONLY, 0600)) == -1) {
        printf("Erro na abertura do arquivo de Log.\n");
        exit(4);
    }

    if(!(yyin = fopen(argv[2], "rt"))) {
        printf("Erro na abertura do arquivo da Requisição.\n");
        exit(2);
    }

    /* Imprime Requisição feita na tela e no arquivo de log */
    dprintf(fd_log, "\n\n\n----- Request -----\n"); // formatacao do logfile
    imprimeConteudo(1,      fileno(yyin)); rewind(yyin);
    imprimeConteudo(fd_log, fileno(yyin)); rewind(yyin);
    dprintf(fd_log, "----- Response Header -----\n"); // formatacao do logfile

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
        status = get(argv[1], comandos->options->option, &fd, filename);
        cabecalho(status, connectionState, filename, fd, fd_resp, fd_log);
        imprimeConteudo(fd_resp, fd);
        if(fd != -1) close(fd);
        break;

        case HEAD:
        status = get(argv[1], comandos->options->option, &fd, filename);
        cabecalho(status, connectionState, filename, fd, fd_resp, fd_log);
        if(fd != -1) close(fd);
        break;

        case OPTIONS:
        dupPrintf(fd_resp, fd_log, "HTTP/1.1 200 OK\r\n");
        dupPrintf(fd_resp, fd_log, "Allow: GET, HEAD, OPTIONS, TRACE\r\n");
        cabecalho(-1, connectionState, filename, -1, fd_resp, fd_log);
        break;

        case TRACE:
        dupPrintf(fd_resp, fd_log, "HTTP/1.1 200 OK\r\n");
        imprimeComandos(comandos->prox, fd_resp, fd_log);
        break;

        case INVALID:
        cabecalho(400, "close", filename, -1, fd_resp, fd_log); // Bad Request
        break;

        default:
        cabecalho(501, "close", filename, -1, fd_resp, fd_log); // Not Implemented
        break;
    }

    dprintf(fd_log, "----- End -----\n"); // formatacao do logfile

    /* libera toda a memoria alocada para as listas ligadas e as strings. */
    liberaComandos(comandos);

    /* Fecha os arquivos */
    close(fd_resp);
    close(fd_log);
    fclose(yyin);

    return 0;
}
