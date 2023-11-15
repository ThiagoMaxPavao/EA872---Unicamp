#include <stdlib.h> // para alocacao dinamica de memoria
#include <stdio.h>
#include "util.h"
#include <sys/types.h> // para arquivos
#include <sys/stat.h>
#include <unistd.h>
#include <time.h> // imprimir data/hora
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include "base64.h"

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
            return "text/html; charset=utf-8";
        } else if(strcmp(ext, "css") == 0) {
            return "text/css; charset=utf-8";
        } else if(strcmp(ext, "txt") == 0) {
            return "text/txt; charset=utf-8";
        } else if(strcmp(ext, "js") == 0) {
            return "application/javascript; charset=utf-8";
        } else if(strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
            return "image/jpeg";
        } else if(strcmp(ext, "png") == 0) {
            return "image/png";
        } else if(strcmp(ext, "pdf") == 0) {
            return "application/pdf";
        }
    }

    return "text/plain; charset=utf-8"; // Padrão
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
        case 503: return "Service Unavailable";
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
    dupPrintf(fd_resp, fd_log, "Server: Servidor HTTP ver. 0.1 de Thiago Maximo Pavao\r\n");
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
    
    /* linha em branco de finalização do header */
    dupPrintf(fd_resp, fd_log, "\r\n");
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

int stringEndsWith(const char * str, const char * suffix) {
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return 
    (str_len >= suffix_len) &&
    (0 == strcmp(str + (str_len-suffix_len), suffix));
}

int isPathConfined(char resource[]) {
    int current_level = 0; // 0-> raiz, >0 dentro de alguma pasta no webspace, <0 para tras do webspace
    int lowest_level = 0; // <0 fora do webspace
    
    for(int i = 0; resource[i] != 0; i++) {
        if(resource[i] == '/' && i == 0) continue;
        if(resource[i] == '/' && resource[i-1] == '/') continue;
        if(resource[i] == '/' && resource[i-1] == '.') continue;
        if(resource[i] == '/' && resource[i-1] != '/') current_level++;
        if(resource[i] == '.' && resource[i-1] == '.') current_level--;
        if(current_level < lowest_level) lowest_level = current_level;
    }

    return lowest_level >= 0;
}

char* join_paths(char* destination, char* path1, char* path2) {
    char c;
    int i = 0;

    /* copia path 1 em destination */
    while((c = path1[i]) != 0) destination[i++] = c;

    /* Adiciona barra entre caminhos */
    destination[i++] = '/';

    /* Concatena path2 */
    for(int j = 0; (c = path2[j]) != 0; j++) destination[i++] = c;
    destination[i] = 0; // finalizador de string

    return destination;
}

void getFilename(char* path, char *name) {
    char* aux = strrchr(path, '/') + 1;
    strcpy(name, aux);
}

char* getParameter(p_no_command comandos_local, char* parameter) {
    for(p_no_command comando = comandos_local; comando != NULL; comando = comando->prox)
        if(strcmp(comando->command, parameter) == 0)
            return comando->options->option;
    return NULL;
}

int hasAuthentication(char* webspace, char* resource, int* authFd) {
    char aux_path[127];
    char htaccess_path[127];
    char password_path[127];
    int htaccessFd;
    char *resourceEnd;
    int find = 0;
    int n_read;

    // inicializa resourceEnd, encontrando o caractere nulo.
    for(resourceEnd = resource; *resourceEnd != 0; resourceEnd++);

    // procura o arquivo htaccess, começando do diretório mais próximo do recurso pedido
    // e voltando, até encontrá-lo ou chegar na raiz.
    while(!find && resourceEnd != resource) {
        // monta o caminho do arquivo htaccess
        join_paths(aux_path, webspace, resource);
        join_paths(htaccess_path, aux_path, ".htaccess");

        // verifica se o arquivo existe
        if(access(htaccess_path, F_OK) == 0) find = 1;

        // Altera o final da string resource, 'voltando' uma pasta
        do { resourceEnd--; } while(*resourceEnd != '/' && resourceEnd != resource);
        *resourceEnd = 0;
    }

    if(!find) return 0;

    if((htaccessFd = open(htaccess_path, O_RDONLY)) == -1) {
        return -1;
    }

    // Lê o caminho para o arquivo de senhas
    n_read = read(htaccessFd, password_path, sizeof(password_path) - 1);
    close(htaccessFd);

    // Terminador de string
    password_path[n_read] = 0;

    if((*authFd = open(password_path, O_RDONLY)) == -1) {
        return -1;
    }

    return 1;
}

/*
Retorna uma linha de fd em buffer,
Retorna 0 se tiver terminado o arquivo e 1 se ainda houver mais coisas para ler nele
*/
int getLine(int fd, char* buffer) {
    static char auxBuffer[1000];
    int n_read, i;

    if (auxBuffer[0] == 0) { // buffer vazio
        n_read = read(fd, auxBuffer, sizeof(auxBuffer) - 1);
        auxBuffer[n_read] = 0;
    } else {
        for (i = 0; auxBuffer[i] != 0 && auxBuffer[i] != '\n'; i++);
        if (auxBuffer[i] == 0) { // buffer sem uma linha completa, provavelmente
            n_read = read(fd, auxBuffer + i, sizeof(auxBuffer) - i - 1);
            auxBuffer[n_read + i] = 0;
        }
    }

    // aqui, com certeza há pelo menos uma linha no buffer.
    // basta copiá-la e depois trazer os caracteres restantes para o começo do buffer
    for (i = 0; auxBuffer[i] != 0 && auxBuffer[i] != '\n'; i++);

    if (auxBuffer[i] == 0) { // só há essa linha e nada mais, fim do arquivo
        strcpy(buffer, auxBuffer);
        return 0;
    } else {
        auxBuffer[i] = 0;
        strcpy(buffer, auxBuffer);
        for (int j = i + 1; auxBuffer[j] != 0; j++)
            auxBuffer[j - (i + 1)] = auxBuffer[j];
        return 1;
    }
}

int hasPermission(int authFd, char *authBase64) {
    char user[127], password[127];
    char userAuth[127], passwordAuth[127];
    char cripto_salt[127];
    int cripto_n;
    char *passwordCripto;
    char authBuffer[1000];
    char *decodedAuthPassword;
    char *decodedAuth;
    char *password
    int decodedAuthSize;
    int n_read;
    int userFound = 0;
    int keepReading = 1;
    match = 0;

    // decodifica autenticacao enviada, formato user:password
    decodedAuth = base64_decode(authBase64, strlen(authBase64), &decodedAuthSize);
    decodedAuth[decodedAuthSize] = 0;

    // separa os valores user e password em suas variáveis
    for(decodedAuthPassword = decodedAuth; decodedAuthPassword != ':'; decodedAuthPassword++);
    decodedAuthPassword = 0;
    decodedAuthPassword++;
    strcpy(user, decodedAuth);
    strcpy(password, decodedAuthPassword);
    free(decodedAuth);

    // percorre usuarios do arquivo até achar o correspondente
    while(keepReading && !userFound) {
        keepReading = getLine(authFd, authBuffer);
        sscanf(authBuffer, "%s:%s", userAuth, passwordAuth);
        if(strcmp(userAuth, user) == 0) userFound = 1;
    }

    if(!userFound) return 0;

    sscanf(passwordAuth, "$%d$%s", &cripto_n, cripto_salt);
    passwordCripto = crypt(cripto_n, cripto_salt);

    if(strcmp(password, passwordCripto) == 0) {
        match = 1;
    }

    free(passwordCripto);
    return match;
}
