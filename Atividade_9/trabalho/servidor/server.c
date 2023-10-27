#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "util.h"
#include <string.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>

extern int get(char* webspace, char* resource, int* fd, char* filename); // funcao para acesso facil ao sistema de arquivos
extern p_no_command comandos; // lista de comandos
extern void yy_scan_string(const char* string); // funcao que passa string para o analisador lexico
extern int yyparse(void); // funcao que faz o parsing
int fd_log, socket_server;

int N_child = 3; // maximo de processos-filho abertos simultaneamente
int n_child = 0; // contador de processos-filho em execução

/*
Interage com o socket definido por socket_msg, esperando que haja conteúdo para ser lido nele e realizando a leitura da requisição.
Esta requisição é então respondida, acessando os arquivos do diretório webspace.
A resposta é enviada em completo para o cliente da requisição e a requisição juntamente com o cabecalho da resposta é adicionado ao
arquivo de registro definido por fd_log.
Vale destacar que a função realiza todas as ações relacionadas a responder uma requisição e APENAS uma.

Retorno:
0: Tudo OK, manter conexão
-1: timeout atingido, requisição não enviada dentro do período aceitável de timeout
1: Requisição respondida com sucesso. Fechar a conexão.

Este último caso ocorre quando o cliente envia no cabecalho da requisição o campo Connection: close ou quando detecta-se erro na
requisição realizada. O que ocorre quando a resposta é 400 Bad Request
*/
int processRequest(char *webspace, int socket_msg, int fd_log) {
    char filename[50], *connectionState;
    char request[1024];
    int bytesRead = 0;
    int n_read;
    int ready;
    int closeConnection = 0;
    int parseStatus;
    int timeoutMs = 10e3;
    
    /* Configura struct pollfd para realizar a chamada poll com o socket e evento de leitura */
    struct pollfd fds;
    fds.fd = socket_msg;
    fds.events = POLLIN;

    /*
    Realiza leituras em loop até que se detecte uma requisição completa,
    identificada pela terminação de duas quebras de linha seguidas 
    */
    do {
        ready = poll(&fds, 1, timeoutMs); // que seja possível ler o socket, esperando no maximo por timeoutMs millisegundos

        if(ready <= 0) return -1; // código de erro para indicar que conexão aberta não enviou conteúdo

        bytesRead += read(socket_msg, request + bytesRead, sizeof(request) - bytesRead - 1);
        request[bytesRead] = 0;
    } while(!stringEndsWith(request, "\r\n\r\n"));

    printf("Requisição recebida por processo %d:\n%s", getpid(), request);

    /* Imprime Requisição feita na tela e no arquivo de log */
    dprintf(fd_log, "\n\n\n----- Request PID = %d -----\n", getpid()); // formatacao do logfile
    dprintf(fd_log, "%s", request);
    dprintf(fd_log, "----- Response Header -----\n"); // formatacao do logfile

    yy_scan_string(request);
    parseStatus = yyparse(); // realiza o parsing, montando a lista ligada

    if(parseStatus == 1) {
        cabecalho(400, "close", filename, -1, socket_msg, fd_log); // Bad Request
        return -2;
    }

    /* 
        Percorre a lista ligada até encontrar um nó que tenha o comando Connection.
        Salva o valor de seu parametro para enviar na resposta posteriormente, mantendo
        a mesma definida pelo cliente em sua requisição.
    */
    for(p_no_command comando = comandos; comando != NULL; comando = comando->prox) {
        if(strcmp(comando->command, "Connection") == 0) {
            connectionState = comando->options->option;
            if(strcmp(connectionState, "close") == 0) closeConnection = 1; 
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
        closeConnection = 1;
        break;

        default:
        cabecalho(501, connectionState, filename, -1, socket_msg, fd_log); // Not Implemented
        break;
    }

    dprintf(fd_log, "----- End -----\n"); // formatacao do logfile

    /* libera toda a memoria alocada para as listas ligadas e as strings. */
    liberaComandos(comandos);
    comandos = NULL;

    // closeConnection = 0 se não houver "Connection=close" na requisição e não houve erro no processamento (erro 400)
    // caso contrário, closeConnection = 1
    return closeConnection;
}

void closeServer() {
    printf("\nDesligando servidor e saindo...\n");

    close(socket_server); // Fecha servidor
    close(fd_log); // Fecha arquivo de log
    exit(0);
}

/*
Função de tratamento de sinal de processos-filho
Verifica se os processos terminaram, decrementando o contador de processos-filho em execução
Também fecha o arquivo de socket que eles estavam trabalhando, pois eles retornam este valor como código de saída.
*/
void processChildProcessSignal(){
    int pid;
    int estado;

    do{
        pid = wait3(&estado,WNOHANG,NULL);
        if(pid > 0) {
            n_child--;
            close(estado>>8); // fecha a conexão com o cliente
        }
    } while(pid > 0);
}

/*
Função de envio padrão de resposta de servidor ocupado.
Acessa o arquivo error_503.html na base do webspace.
Envia para
*/
void respostaPadraoOcupado(char *webspace, int socket_msg, int fd_log) {
    int fd;
    char filename[50];

    printf("\nRequisição recebida, servidor ocupado.\n");

    dprintf(fd_log, "\n\n\n----- Request recebido mas sem disponibilidade para atender, enviando mensagem padrão e página informativa -----\n");
    dprintf(fd_log, "----- Response Header feito pelo processo pai -----\n");

    get(webspace, "error_503.html", &fd, filename);
    cabecalho(503, "close", filename, fd, socket_msg, fd_log);
    imprimeConteudo(socket_msg, fd);
}

int main(int argc, char *argv[]) {
    int socket_msg;
    struct sockaddr_in server, client;
    int addr_size = sizeof(client);
    char *webspace, *logFilename;
    int portNumber;
    
    /* Verifica formato da chamada */
    if(argc != 4) {
        printf("Uso: %s <Web Space> <Porta> <Arquivo de Log>\n", argv[0]);
        exit(1);
    }

    /*
        Realiza a leitura dos parâmetros
    */
    webspace = argv[1];
    portNumber = atoi(argv[2]);
    logFilename = argv[3];

    /* Abre o arquivo de log, indicando se houver erro */
    if((fd_log = open(logFilename, O_APPEND | O_CREAT | O_WRONLY, 0600)) == -1) {
        perror("Erro na abertura do arquivo de Log");
        exit(2);
    }

    /* Inicializa o socket do servidor */
    if((socket_server = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erro na criação do socket");
        exit(3);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(portNumber);
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
        para closeServer, que fecha os arquivos e então encerra a execuçao
    */
    signal(SIGINT, closeServer);

    /*
        Configura função para atualizar numero de processos quando um termina
        captuando o sinal SIGCHLD
    */
    signal(SIGCHLD, processChildProcessSignal);

    printf("Servidor iniciado e ouvindo na porta %d\n", portNumber);

    while(1) {
        socket_msg = accept(socket_server, (struct sockaddr*)&client, &addr_size);
        
        if(socket_msg == -1) {
            if(errno == EINTR) {
                printf("\naccept interrompido por algum sinal, executando novamente...\n");
                continue; // se saiu do accept por causa de interrupcao, volta no comeco do loop
            }
            else {
                perror("Erro em accept");
                exit(1);
            }
        }

        if(n_child == N_child) {
            respostaPadraoOcupado(webspace, socket_msg, fd_log);
            close(socket_msg);
            continue;
        }

        switch(fork()) {
			case -1: // erro
                perror("Erro em fork para processamento de requisicao");
                exit(0);
			case 0: // processo filho
                printf("\nProcesso filho %d inicializado, conexão com socket %d\n", getpid(), socket_msg);
                /*
                Processa requisições até que ela retorne algo diferente de 0, indicando que a conexão deve ser fechada
                */
                while(processRequest(webspace, socket_msg, fd_log) == 0);
                close(socket_msg);
                printf("\nProcesso filho %d encerrando, socket %d\n", getpid(), socket_msg);
                exit(socket_msg);
			default: // processo pai
                n_child++;
                break;
		}

    }
}
