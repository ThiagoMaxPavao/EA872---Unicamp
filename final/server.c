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
#include <pthread.h>
#include "base64.h"
#include "get.h"

extern p_no_command comandos; // lista de comandos
extern void yy_scan_string(const char* string); // funcao que passa string para o analisador lexico
extern int yyparse(void); // funcao que faz o parsing
int fd_log, socket_server;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t parsing_mutex = PTHREAD_MUTEX_INITIALIZER;

int N_threads;     // maximo de threads executando simultaneamente
int n_threads = 0; // contador de threads em execução

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
    char filename[50], *url, *connectionState, *authBase64;
    char request[1024]; // cabecalho da requisição
    char content[1024]; // corpo da requisição
    char *contentLengthHeader;
    int contentLength = 0;
    int bytesRead = 0;
    int n_read;
    int ready;
    int closeConnection = 0;
    int parseStatus;
    int timeoutMs = 5000;
    p_no_command comandos_local;
    Metodo metodo;
    
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

        n_read = read(socket_msg, request + bytesRead, sizeof(request) - bytesRead - 1);

        if(n_read <= 0) return -1; // EOF

        bytesRead += n_read;
        request[bytesRead] = 0;
    } while(strstr(request, "\r\n\r\n") == NULL); // loop enquanto não encontra o fim de um header

    char *headerEnd = strstr(request, "\r\n\r\n");
    headerEnd[2] = 0; // coloca o finalizador da string de request

    // REGIÃO CRÍTICA -> flex e bison utilizam variáveis globais
    pthread_mutex_lock(&parsing_mutex);
    yy_scan_string(request);
    parseStatus = yyparse(); // realiza o parsing, montando a lista ligada
    comandos_local = comandos; // copia lista ligada para varíavel local
    comandos = NULL; // limpa lista global para poder ser utilizada por qualquer thread
    pthread_mutex_unlock(&parsing_mutex);

    contentLengthHeader = getParameter(comandos_local, "Content-Length");
    contentLength = contentLengthHeader == NULL ? 0 : atoi(contentLengthHeader);
    if(contentLength != 0) {
        strcpy(content, headerEnd+4);
        bytesRead = strlen(content);
        read(socket_msg, content + bytesRead, contentLength - bytesRead);
    }

    /* Imprime Requisição recebida na tela e no arquivo de log */
    printf("Requisição recebida por thread %ld:\n%s\n", pthread_self(), request);
    if(contentLength != 0) printf("Corpo da requisição:\n%s\n", content);

    dprintf(fd_log, "\n\n\n----- Request Header - Thread = %ld -----\n", pthread_self()); // formatacao do logfile
    dprintf(fd_log, "%s\n", request);
    if(contentLength != 0) {
        dprintf(fd_log, "----- Request Content - Thread = %ld -----\n", pthread_self()); // formatacao do logfile
        dprintf(fd_log, "%s\n\n", content);
    }
    dprintf(fd_log, "----- Response Header -----\n"); // formatacao do logfile

    if(parseStatus == 1) {
        cabecalho(400, "close", filename, NULL, -1, socket_msg, fd_log); // Bad Request
        return -2;
    }

    /* 
        Salva do parametro Connection para enviar na resposta posteriormente, mantendo
        a mesma definida pelo cliente em sua requisição.
    */
    connectionState = getParameter(comandos_local, "Connection");
    if(strcmp(connectionState, "close") == 0) closeConnection = 1;
    
    /*
    Salva referencia para par usuario:senha
    + 6 -> para pular o "Basic "
    */
    authBase64 = getParameter(comandos_local, "Authorization");
    if(authBase64 != NULL) authBase64 += 6;

    /* Trata cada caso de método, incluindo o caso de bad request e not implemented */
    int fd, status;
    switch(metodo = stringParaMetodo(comandos_local->command)) {
        case HEAD:
        case GET:
        url = comandos_local->options->option;
        status = get(webspace, url, &fd, filename, authBase64);
        cabecalho(status, connectionState, filename, url, fd, socket_msg, fd_log);
        if(fd != -1) {
            if(metodo == GET) imprimeConteudo(socket_msg, fd);
            close(fd);
        }
        break;

        case OPTIONS:
        dupPrintf(socket_msg, fd_log, "HTTP/1.1 200 OK\r\n");
        dupPrintf(socket_msg, fd_log, "Allow: GET, HEAD, OPTIONS, TRACE\r\n");
        cabecalho(-1, connectionState, filename, NULL, -1, socket_msg, fd_log);
        break;

        case TRACE:
        dupPrintf(socket_msg, fd_log, "HTTP/1.1 200 OK\r\n");
        imprimeComandos(comandos_local->prox, socket_msg, fd_log);
        break;

        case INVALID:
        cabecalho(400, "close", filename, NULL, -1, socket_msg, fd_log); // Bad Request
        closeConnection = 1;
        break;

        default:
        cabecalho(501, connectionState, filename, NULL, -1, socket_msg, fd_log); // Not Implemented
        break;
    }

    dprintf(fd_log, "----- End -----\n"); // formatacao do logfile

    /* libera toda a memoria alocada para as listas ligadas e as strings. */
    liberaComandos(comandos_local);

    // closeConnection = 0 se não houver "Connection=close" na requisição e não houve erro no processamento (erro 400)
    // caso contrário, closeConnection = 1
    return closeConnection;
}

/*
Função da thread, atende as requisições de uma conexão até que ela seja encerrada
*/
void * HandleConnection(void * data) {
    thread_input_data_ptr typed_data = (thread_input_data_ptr) data;
    char *webspace = typed_data->webspace;
    int socket_msg = typed_data->socket_msg;
    int fd_log = typed_data->fd_log;
    free(typed_data);

    printf("\nThread %ld inicializado, conexão com socket %d\n", pthread_self(), socket_msg);
    
    /*
        Processa requisições até que a função retorne algo diferente de 0,
        indicando que a conexão deve ser fechada
    */
    while(processRequest(webspace, socket_msg, fd_log) == 0);
    close(socket_msg);
    printf("\nThread %ld encerrando, socket %d\n", pthread_self(), socket_msg);

    // REGIÃO CRÍTICA -> acesso e modificação de variável global de contagam
    pthread_mutex_lock(&count_mutex);
    n_threads--;
    pthread_mutex_unlock(&count_mutex);

    pthread_exit(NULL);
}

void closeServer() {
    printf("\nDesligando servidor e saindo...\n");

    base64_cleanup(); // libera tabela de (de)codificação base64
    close(socket_server); // Fecha servidor
    close(fd_log); // Fecha arquivo de log
    exit(0);
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
    dprintf(fd_log, "----- Response Header feito pela Thread principal -----\n");
    
    openAndReturnError(503,  &fd, filename);
    cabecalho(503, "close", filename, NULL, fd, socket_msg, fd_log);
    imprimeConteudo(socket_msg, fd);
}

int main(int argc, char *argv[]) {
    int socket_msg;
    struct sockaddr_in server, client;
    int addr_size = sizeof(client);
    char *webspace, *logFilename;
    int portNumber;
    pthread_attr_t attr;
    thread_input_data_ptr data;
    int rc;
    int ocupado;
    pthread_t tid;
    
    /* Verifica formato da chamada */
    if(argc != 5) {
        printf("Uso: %s <Web Space> <Porta> <Arquivo de Log> <Max threads>\n", argv[0]);
        exit(1);
    }

    // Inicializa tabela para (de)codificação base64.
    build_decoding_table();

    /*
        Realiza a leitura dos parâmetros
    */
    configureErrorPagesPath(argv[0]);
    webspace = argv[1];
    portNumber = atoi(argv[2]);
    logFilename = argv[3];
    N_threads = atoi(argv[4]);

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

    /* Atributo de criação de thread sem sincronização */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

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

        // REGIÃO CRÍTICA -> acesso e modificação de variável global de contagam
        pthread_mutex_lock(&count_mutex);
        if(n_threads == N_threads) ocupado = 1;
        else {
            ocupado = 0;
            n_threads++;
        }
        pthread_mutex_unlock(&count_mutex);

        if(ocupado) {
            respostaPadraoOcupado(webspace, socket_msg, fd_log);
            close(socket_msg);
            continue; // volta para esperar novas requisições
        }

        // Configura dados de passagem para a thread
        data = malloc(sizeof(thread_input_data));
        data->webspace = webspace;
        data->socket_msg = socket_msg;
        data->fd_log = fd_log;
        if(rc = pthread_create(&tid, &attr, HandleConnection, (void *)data)) {
            printf("\nERRO: codigo de retorno de pthread_create é %d \n", rc);
            exit(1);
        }

    }
}
