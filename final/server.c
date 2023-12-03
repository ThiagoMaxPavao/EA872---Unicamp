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
#include "sint.tab.h"
#include "lex.yy.h"

int fd_log, socket_server;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

int N_threads;     // maximo de threads executando simultaneamente
int n_threads = 0; // contador de threads em execução

char *changePasswordFilename;

/*
Interage com o socket definido por socket_msg, esperando que haja conteúdo para ser lido nele e realizando a leitura da requisição.
Esta requisição é então respondida, acessando os arquivos do diretório webspace.
A resposta é enviada em completo para o cliente da requisição e a requisição juntamente com o cabecalho da resposta é adicionado ao
arquivo de registro definido por fd_log.
Vale destacar que a função realiza todas as ações relacionadas a responder uma requisição e APENAS uma.

Retorno:
0: Tudo OK, manter conexão
-1: requisição não enviada dentro do período aceitável de timeout
    ou erro de sintaxe no parsing do cabeçalho da requisição.
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
    p_no_command comandos_local = NULL;
    Metodo metodo;
    yyscan_t scanner;
    
    /* Configura struct pollfd para realizar a chamada poll com o socket e evento de leitura */
    struct pollfd fds;
    fds.fd = socket_msg;
    fds.events = POLLIN;

    /*
    Realiza leituras em loop até que se detecte uma requisição completa,
    identificada pela terminação de duas quebras de linha seguidas 
    */
    do {
        // aguarda que seja possível ler o socket, esperando no maximo por timeoutMs millisegundos
        ready = poll(&fds, 1, timeoutMs);

        if(ready <= 0) return -1; // código de erro para indicar que conexão aberta não enviou conteúdo

        n_read = read(socket_msg, request + bytesRead, sizeof(request) - bytesRead - 1);

        if(n_read <= 0) return -1; // EOF

        bytesRead += n_read;
        request[bytesRead] = 0;
    } while(strstr(request, "\r\n\r\n") == NULL); // loop enquanto não encontra o fim de um header

    char *headerEnd = strstr(request, "\r\n\r\n");
    headerEnd[2] = 0; // coloca o finalizador da string de request

    yylex_init(&scanner);
    yy_scan_string(request, scanner);
    parseStatus = yyparse(scanner, &comandos_local); // realiza o parsing, montando a lista ligada
    yylex_destroy(scanner);

    /* Imprime cabeçalho da requisição recebida na tela e no arquivo de log */
    printf("Requisição recebida por thread %ld:\n%s\n", pthread_self(), request);

    dprintf(fd_log, "\n\n\n----- Request Header - Thread = %ld -----\n", pthread_self()); // formatacao do logfile
    dprintf(fd_log, "%s\n", request);

    /* Verifica o parâmetro Content-Length para saber se a requisição conta com corpo */
    contentLengthHeader = getParameter(comandos_local, "Content-Length");
    contentLength = contentLengthHeader == NULL ? 0 : atoi(contentLengthHeader);

    if(contentLength > sizeof(content)) { // corpo grande demais
        liberaComandos(comandos_local); // Libera memória que pode ter sido alocada
        cabecalho(413, "close", filename, NULL, -1, socket_msg, fd_log); // Payload Too Large
        return -1;
    }
    else if(contentLength > 0) {
        strcpy(content, headerEnd+4);
        bytesRead = strlen(content);
        
        while(bytesRead < contentLength) {
            // aguarda que seja possível ler o socket, esperando no maximo por timeoutMs millisegundos
            ready = poll(&fds, 1, timeoutMs);
            
            if(ready <= 0) return -1; // código de erro para indicar que conexão aberta não enviou conteúdo

            bytesRead += read(socket_msg, content + bytesRead, contentLength - bytesRead);
        }

        printf("Corpo da requisição:\n%s\n", content);
        
        dprintf(fd_log, "----- Request Content - Thread = %ld -----\n", pthread_self()); // formatacao do logfile
        dprintf(fd_log, "%s\n\n", content);
    }

    dprintf(fd_log, "----- Response Header -----\n"); // formatacao do logfile

    /* Erro de sintaxe no cabeçalho -> Requisição com problemas = Bad Request */
    if(parseStatus == 1) {
        liberaComandos(comandos_local); // Libera memória que pode ter sido alocada
        cabecalho(400, "close", filename, NULL, -1, socket_msg, fd_log); // Bad Request
        return -1;
    }

    /* 
        Salva valor do parametro Connection para enviar na resposta posteriormente,
        mantendo a mesma definida pelo cliente em sua requisição.
    */
    connectionState = getParameter(comandos_local, "Connection");
    if(connectionState == NULL) connectionState = "keep-alive";
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
            /* Só envia o arquivo se for uma requisição do tipo GET */
            if(metodo == GET) imprimeConteudo(socket_msg, fd);
            close(fd);
        }
        break;

        case OPTIONS:
        url = comandos_local->options->option;
        dupPrintf(socket_msg, fd_log, "HTTP/1.1 200 OK\r\n");
        /* Inclui o POST nas opções, se a URL for de troca de senha */
        dupPrintf(socket_msg, fd_log, stringEndsWith(url, changePasswordFilename) ? 
        "Allow: GET, HEAD, POST, OPTIONS, TRACE\r\n" : "Allow: GET, HEAD, OPTIONS, TRACE\r\n");
        cabecalho(-1, connectionState, filename, NULL, -1, socket_msg, fd_log);
        break;

        case TRACE:
        dupPrintf(socket_msg, fd_log, "HTTP/1.1 200 OK\r\n");
        /* Envia a própria requisição de volta, exceto a primeira linha */
        imprimeComandos(comandos_local->prox, socket_msg, fd_log);
        break;

        case INVALID: // método enviado não reconhecido
        cabecalho(400, "close", filename, NULL, -1, socket_msg, fd_log); // Bad Request
        closeConnection = 1;
        break;
        
        case POST:
        url = comandos_local->options->option;
        status = processPost(webspace, url, &fd, filename, content);
        cabecalho(status, connectionState, filename, url, fd, socket_msg, fd_log);
        if(fd != -1) {
            imprimeConteudo(socket_msg, fd);
            close(fd);
        }
        break;

        default:
        cabecalho(501, connectionState, filename, NULL, -1, socket_msg, fd_log); // Not Implemented
        break;
    }

    dprintf(fd_log, "----- End -----\n"); // formatacao do logfile

    /* libera toda a memoria alocada para as listas ligadas e as strings. */
    liberaComandos(comandos_local);

    // closeConnection = 0 se não houver "Connection=close" na requisição e
    // não houve erro no processamento. Caso contrário, closeConnection = 1.
    return closeConnection;
}

/*
Função da thread, atende as requisições de uma conexão até que ela seja encerrada
Recebe dados na forma da struct thread_input_data_ptr, contendo o endereço do
webspace, o socket da conexão e o descritor para o arquivo de log.
*/
void * HandleConnection(void * data) {

    /* Coleta os dados enviados */
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

    /* Fecha a conexão */
    close(socket_msg);
    printf("\nThread %ld encerrando, socket %d\n", pthread_self(), socket_msg);

    // REGIÃO CRÍTICA -> acesso e modificação de variável global de contagam
    pthread_mutex_lock(&count_mutex);
    n_threads--;
    pthread_mutex_unlock(&count_mutex);

    /* Encerra a thread */
    pthread_exit(NULL);
}

void closeServer() {
    printf("\nDesligando servidor e saindo...\n");

    base64_cleanup(); // libera tabela de (de)codificação base64
    close(socket_server); // Fecha servidor, libera porta
    close(fd_log); // Fecha arquivo de log
    exit(0);
}

/*
Função de envio padrão de resposta de servidor ocupado.
Acessa o arquivo error_503.html na base do webspace.
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
    unsigned int addr_size = sizeof(client);
    char *webspace, *logFilename;
    int portNumber;
    pthread_attr_t attr;
    thread_input_data_ptr data;
    int rc;
    int ocupado;
    pthread_t tid;
    
    /* Verifica formato da chamada */
    if(argc != 6 && argc != 7) {
        printf("Uso: %s <Web Space> <Porta> <Arquivo de Log> <URL de troca de senha> <Max threads> [charset (tipo de codificacao)]\n", argv[0]);
        exit(1);
    }

    /* Inicializa tabela para (de)codificação base64. */
    build_decoding_table();

    /* Realiza a leitura dos parâmetros */
    configureServerPagesPath(argv[0]);
    configurePasswordFilesPath(argv[0]);
    webspace = argv[1];
    portNumber = atoi(argv[2]);
    logFilename = argv[3];
    changePasswordFilename = argv[4];
    N_threads = atoi(argv[5]);
    if(argc == 7) configureCharset(argv[6]);

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

    /* Configura server */
    server.sin_family = AF_INET;
    server.sin_port = htons(portNumber);
    server.sin_addr.s_addr = INADDR_ANY;

    /*
    Associa o nome configurado em server (número da porta) ao
    descritor de arquivo do socket.
    */
    if(bind(socket_server, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("Erro em bind");
        exit(4);
    }

    /* Escuta conexões no socket, até 5 são armazenadas em fila */
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
        /* Espera até que algum cliente se conecte */
        socket_msg = accept(socket_server, (struct sockaddr*)&client, &addr_size);
        
        if(socket_msg == -1) { // Verifica erro na saída de accept
            if(errno == EINTR) {
                printf("\naccept interrompido por algum sinal, executando novamente...\n");
                continue; // se saiu do accept por causa de interrupcao, volta no comeco do loop
            }
            else {
                perror("Erro em accept");
                exit(1); // Se saiu por outro erro encerra a execução
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

        if(ocupado) { // resposta padrão e volta para esperar novas requisições
            respostaPadraoOcupado(webspace, socket_msg, fd_log);
            close(socket_msg);
            continue;
        }

        /* Configura dados de passagem para a thread */
        data = malloc(sizeof(thread_input_data));
        data->webspace = webspace;
        data->socket_msg = socket_msg;
        data->fd_log = fd_log;

        /* Cria thread para atender ao cliente, verifica erro na criação */
        if((rc = pthread_create(&tid, &attr, HandleConnection, (void *)data)) != 0) {
            printf("Erro ao criar thread: codigo de retorno de pthread_create é %d\n", rc);
            close(socket_msg); // Encerra a conexão
            continue;
        }

    }
}
