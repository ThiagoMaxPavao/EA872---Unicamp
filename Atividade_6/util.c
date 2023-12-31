#include <stdlib.h> // para alocacao dinamica de memoria
#include <stdio.h>
#include "util.h"
#include <sys/types.h> // para arquivos
#include <sys/stat.h>
#include <unistd.h>
#include <time.h> // imprimir data/hora
#include <string.h>
#include <stdarg.h>

p_no_command criaComando(char* command, p_no_option options) {
    p_no_command novo = malloc(sizeof(no_command));
    novo->command = command; 
    novo->options = options;
    novo->prox = NULL;
    return novo;
}

p_no_option anexaParametro(p_no_option lista, char* option) {
    p_no_option novo = malloc(sizeof(no_option));
    novo->option = option;
    novo->prox = lista;
    return novo;
}

/*
Imprime os parametros armazenados em uma lista ligada de opcoes
se a lista for vazia, avisa que nao ha parametros
separa cada parametro com uma virgula
*/
void imprimeParametros(p_no_option lista, int fd_resp, int fd_log) {
    while(lista != NULL) {
        dupPrintf(fd_resp, fd_log, "%s", lista->option);
        if(lista->prox != NULL) dupPrintf(fd_resp, fd_log, ",");
        lista = lista->prox;
    }
}

void imprimeComandos(p_no_command lista, int fd_resp, int fd_log) {
    while(lista != NULL) {
        dupPrintf(fd_resp, fd_log, "%s: ", lista->command);
        imprimeParametros(lista->options, fd_resp, fd_log);
        dupPrintf(fd_resp, fd_log, "\r\n");
        lista = lista->prox;
    }
}

void imprimeErroSemComando(int lineNumber) {
    fprintf(stderr, "Linha %d sem comando!\n", lineNumber);
}

void liberaOpcoes(p_no_option lista) {
    while (lista != NULL) {
        p_no_option temp = lista;
        lista = lista->prox;
        free(temp->option); // Libera a string
        free(temp);         // Libera o no
    }
}

void liberaComandos(p_no_command lista) {
    while (lista != NULL) {
        p_no_command temp = lista;
        lista = lista->prox;
        free(temp->command);         // Libera a string
        liberaOpcoes(temp->options); // libera a lista de opcoes
        free(temp);                  // Libera o no
    }
}

Metodo stringParaMetodo(char* str) {
    if(strcmp(str, "GET") == 0)     return GET;
    if(strcmp(str, "HEAD") == 0)    return HEAD;
    if(strcmp(str, "OPTIONS") == 0) return OPTIONS;
    if(strcmp(str, "TRACE") == 0)   return TRACE;
    if(strcmp(str, "POST") == 0)    return POST;
    if(strcmp(str, "PUT") == 0)     return PUT;
    if(strcmp(str, "DELETE") == 0)  return DELETE;
    return INVALID;
}

/*
Retorna uma string para o valor de Content-Type da resposta baseando-se no nome do arquivo
Faz a verificação de acordo com o nome do arquivo.
*/
char* getContentType(char* filename) {
    char* ext = strrchr(filename, '.');

    if(ext) {
        ext++; // pula o ponto
        if(strcmp(ext, "html") == 0) {
            return "text/html";
        } else if(strcmp(ext, "css") == 0) {
            return "text/css";
        } else if(strcmp(ext, "txt") == 0) {
            return "text/txt";
        } else if(strcmp(ext, "js") == 0) {
            return "application/javascript";
        } else if(strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
            return "image/jpeg";
        } else if(strcmp(ext, "png") == 0) {
            return "image/png";
        } else if(strcmp(ext, "pdf") == 0) {
            return "application/pdf";
        }
    }

    return "text/plain"; // Padrão
}

/*
Pega a string de status relativa a certo código de status
Por exemplo, 200 -> OK
*/
char* getStatusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default:  return "Unknown Status";
    }
}

void cabecalho(int status, char* connection, char* filename, int fd, int fd_resp, int fd_log) {
    time_t timer;
    char dateBuffer[30];
    struct tm* tm_info;

    /* Constroi a data atual */
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(dateBuffer, 30, "%a %b %d %T %Y BRT", tm_info);
    
    if(status != -1) dupPrintf(fd_resp, fd_log, "HTTP/1.1 %d %s\r\n", status, getStatusText(status));
    dupPrintf(fd_resp, fd_log, "Date: %s\r\n", dateBuffer);
    dupPrintf(fd_resp, fd_log, "Server: Servidor HTTP ver. 0.1 de Thiago Maximo Pavão\r\n");
    dupPrintf(fd_resp, fd_log, "Connection: %s\r\n", connection);

    if(fd != -1) { // Caso haja um arquivo para imprimir o cabecalho
        struct stat statbuf;
        /* Pega informações sobre o arquivo */
        fstat(fd, &statbuf);

        /* Data de ultima modificação */
        tm_info = localtime(&statbuf.st_mtime);
        strftime(dateBuffer, 30, "%a %b %d %T %Y BRT", tm_info);
        dupPrintf(fd_resp, fd_log, "Last-Modified: %s\r\n", dateBuffer);

        dupPrintf(fd_resp, fd_log, "Content-Length: %ld\r\n",statbuf.st_size);
        dupPrintf(fd_resp, fd_log, "Content-Type: %s\r\n", getContentType(filename));
    }
    else {
        dupPrintf(fd_resp, fd_log, "Content-Length: 0\r\n");
        dupPrintf(fd_resp, fd_log, "Content-Type: text/html\r\n");
    }
}

int imprimeConteudo(int destinationFd, int originFd) {
    char buffer[50];
    char bufferCorrigido[2*sizeof(buffer)]; // potencialmente completo de quebras de linha sozinhas
    int n; // n -> numero de bytes lidos por read
    while((n = read(originFd, buffer, sizeof(buffer))) > 0) {
        write(destinationFd, buffer, n);
    }
    return n; // 0 se não houver mais nada para ler, -1 se houve erro na leitura
}

void dupPrintf(int fd1, int fd2, char const *format, ...) { 
    va_list ap;
    va_start(ap, format);
    vdprintf(fd1, format, ap);
    va_end(ap);
    va_start(ap, format);
    vdprintf(fd2, format, ap);
    va_end(ap);
}
