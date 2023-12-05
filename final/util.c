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
#include <crypt.h>

static char serverPasswordsPath[100];

char* allocAndCopy(char* str) {
    int len, i;
    char* new;

    for(; *str == ' '; str++); // pula espacos do comeco da string, se houverem
    for(len = 0; str[len]!=0; len++); // descobre tamanho da string

    new = malloc(len + 1); // aloca vetor de char do mesmo tamanho que a string
                           // (mais um para colocar o \0)

    for(i = 0; i<=len; i++) new[i] = str[i]; // copia a string
    
    return new;
}

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

static char *extToContentTypeTable[] = {
    "bin:application/octet-stream",
    "css:text/css",
    "gif:image/gif",
    "htm:text/html",
    "html:text/html",
    "jpeg:image/jpeg",
    "jpg:image/jpeg",
    "js:text/javascript",
    "json:application/json",
    "mp3:audio/mpeg",
    "mp4:video/mp4",
    "png:image/png",
    "pdf:application/pdf",
    "rar:application/vnd.rar",
    "sh:application/x-sh",
    "svg:image/svg+xml",
    "tar:application/x-tar",
    "tif:image/tiff",
    "tiff:image/tiff",
    "txt:text/plain",
    "zip:application/zip",
    NULL
};

/*
Recebe o nome do arquivo e, a partir de sua extensão, retorna uma string
contendo o Content-Type base, sem o tipo de codificação.
*/
char *getContentTypeBase(char *filename) {
    char* ext = strrchr(filename, '.');
    int i, extLength;
    char *currentType;

    if(ext) {
        ext++; // pula o ponto
        extLength = strlen(ext);

        for(i = 0; (currentType = extToContentTypeTable[i]) != NULL; i++) {
            if(currentType[extLength] != ':') continue;
            if(strncmp(currentType, ext, extLength) != 0) continue;
            return currentType + extLength + 1;
        }
    }

    return "text/plain";
}

static char *charset = NULL;

void configureCharset(char *charset_param) {
    charset = charset_param;
}

/*
Escreve em output a string contendo o valor para enviar com o header Content-Type
Suporta multiplos tipos de arquivos, e adiciona "; charset=<charset configurado>"
Se o arquivo for do tipo texto e o tipo de codificao tiver sido configurado.
*/
void getContentType(char *output, char* filename) {
    strcpy(output, getContentTypeBase(filename));
    if(charset != NULL && strncmp(output, "text", 4) == 0) {
        strcat(output, "; charset=");
        strcat(output, charset);
    }
}

/*
Pega a string de status relativa a certo código de status
Por exemplo, 200 -> OK
*/
char* getStatusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 401: return "Authorization Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default:  return "Unknown Status";
    }
}

void cabecalho(int status, char *connection, char *filename, char *realm, int fd, int fd_resp, int fd_log) {
    time_t timer;
    char dateBuffer[32];
    char contentTypeBuffer[32];
    struct tm* tm_info;

    /* Constroi a data atual */
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(dateBuffer, 30, "%a %b %d %T %Y BRT", tm_info);
    
    if(status != -1) dupPrintf(fd_resp, fd_log, "HTTP/1.1 %d %s\r\n", status, getStatusText(status));
    dupPrintf(fd_resp, fd_log, "Date: %s\r\n", dateBuffer);
    dupPrintf(fd_resp, fd_log, "Server: Servidor HTTP ver. 1.0 de Thiago Maximo Pavao\r\n");
    if(status == 401) {
        /* Envia cabeçalho requisitando autenticação, caso status = 401 Authorization Required*/
        dupPrintf(fd_resp, fd_log, "WWW-Authenticate: Basic realm=\"%s\"\r\n", realm);
    }
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
        
        getContentType(contentTypeBuffer, filename);
        dupPrintf(fd_resp, fd_log, "Content-Type: %s\r\n", contentTypeBuffer);
    }
    else {
        /* Não há arquivo sendo enviado */
        dupPrintf(fd_resp, fd_log, "Content-Length: 0\r\n");
        dupPrintf(fd_resp, fd_log, "Content-Type: text/html\r\n");
    }
    
    /* linha em branco de finalização do header */
    dupPrintf(fd_resp, fd_log, "\r\n");
}

int imprimeConteudo(int destinationFd, int originFd) {
    char buffer[50];
    
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

int hasAuthentication(char* webspace, char* resource_parameter, int* authFd) {
    char aux_path[127];
    char resource[127];
    char htaccess_path[127];
    char password_path[127];
    int htaccessFd;
    char *resourceEnd;
    int find = 0;
    int n_read;

    /* copia recurso desejado para array local */
    strcpy(resource, resource_parameter);

    /* inicializa resourceEnd, encontrando o caractere nulo. */
    for(resourceEnd = resource; *resourceEnd != 0; resourceEnd++);

    /*
    procura o arquivo htaccess, começando do diretório mais próximo do recurso pedido
    e voltando, até encontrá-lo ou chegar na raiz.
    */
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

    /* Não encontrou nenhum .htaccess na árvore do recurso, recurso livre */
    if(!find) return 0;

    if((htaccessFd = open(htaccess_path, O_RDONLY)) == -1) {
        return -1;
    }

    /* Lê o caminho para o arquivo de senhas */
    n_read = read(htaccessFd, password_path, sizeof(password_path) - 1);
    close(htaccessFd);

    /* Terminador de string removendo possível quebra de linha */
    if(password_path[n_read - 1] == '\n') n_read--;
    if(password_path[n_read - 1] == '\r') n_read--;
    password_path[n_read] = 0;
    
    /*
    Verifica se o endereço é relativo, neste caso acessa relativo à pasta
    passwords no mesmo diretório do programa.
    */
    if(password_path[0] != '/') {
        strcpy(aux_path, password_path);
        join_paths(password_path, serverPasswordsPath, aux_path);
    }

    /* Abre o arquivo de senhas obtido */
    if((*authFd = open(password_path, O_RDWR)) == -1) {
        return -1;
    }

    return 1;
}

int getLine(int fd, char* buffer, char *auxBuffer, int auxBufferSize) {
    int n_read, i, j;
    int lineSize;

    if (auxBuffer[0] == 0) { // buffer vazio
        n_read = read(fd, auxBuffer, auxBufferSize - 1);
        if(n_read == 0) return -1; // fim do arquivo
        auxBuffer[n_read] = 0;
    } else {
        for (i = 0; auxBuffer[i] != 0 && auxBuffer[i] != '\r' && auxBuffer[i] != '\n'; i++);
        if (auxBuffer[i] == 0) { // buffer sem uma linha completa
            n_read = read(fd, auxBuffer + i, auxBufferSize - i - 1);
            auxBuffer[n_read + i] = 0;
        }
    }

    // aqui, com certeza há pelo menos uma linha no buffer.
    // basta copiá-la e depois trazer os caracteres restantes para o começo do buffer
    for (i = 0; auxBuffer[i] != 0 && auxBuffer[i] != '\r' && auxBuffer[i] != '\n'; i++);

    lineSize = i;
    if(auxBuffer[lineSize] == '\r') lineSize++;
    if(auxBuffer[lineSize] == '\n') lineSize++;

    auxBuffer[i] = 0;
    strcpy(buffer, auxBuffer);
    for (j = lineSize; auxBuffer[j] != 0; j++)
        auxBuffer[j - lineSize] = auxBuffer[j];
    auxBuffer[j - lineSize] = 0;

    return lineSize;
}

int hasPermission(int authFd, char *user, char *password, char *cripto_salt_output, int *position_output) {
    char userAuth[127], passwordAuth[127];
    char cripto_salt[127];
    char *passwordCripto;
    char authBuffer[100];
    char getLineAuxBuffer[200] = "";
    char c;
    int userFound = 0;
    int i;
    int cifraoCount = 0;
    int position = 0;
    int lineSize;

    /* percorre usuarios do arquivo até achar o correspondente */
    while(((lineSize = getLine(authFd, authBuffer, getLineAuxBuffer, 200)) >= 0) && !userFound) {
        position += lineSize;
        if(strlen(authBuffer) == 0) continue; // ignora linhas em branco

        /* encontra o caractere de separação ':' */
        for(i = 0; authBuffer[i] != ':' && authBuffer[i] != 0; i++);
        if(authBuffer[i] == 0) continue;

        authBuffer[i] = 0;
        strcpy(userAuth, authBuffer);
        strcpy(passwordAuth, authBuffer + i + 1);

        /* Verifica se o usuário é o que foi informado */
        if(strcmp(userAuth, user) == 0) {
            userFound = 1;
            position -= lineSize;
        }
    }

    /* nome de usuario não encontrado */
    if(!userFound) return 0;

    /* Copia o salt da senha */
    for(i = 0; cifraoCount < 3 && ((c = passwordAuth[i]) != 0); i++) {
        cripto_salt[i] = c;
        if(c == '$') cifraoCount++;
    }
    cripto_salt[i] = 0;

    // Caso não tenham '$'s na senha, então foi utilizada criptografia default
    // Neste caso copia o salt, que são os dois primeiros caracteres da senha criptografada
    if(c == 0 && cifraoCount != 0) return 0;
    else if(cifraoCount == 0) {
        cripto_salt[0] = passwordAuth[0];
        cripto_salt[1] = passwordAuth[1];
        cripto_salt[2] = 0;
    }

    /* Criptografa a senha informada utilizando o sal da armazenada no hypassword */
    passwordCripto = crypt(password, cripto_salt);

    // compara passwordAuth -> vindo do arquivo de senhas com passwordCripto ->
    // gerado agora utilizando o salt do usuario encontrado no arquivo de senhas
    // e a senha informada pelo cliente da requisição.
    if(!strcmp(passwordAuth, passwordCripto) == 0) return 0; // senha inválida

    /* Seta parametros de saída com os valores esperados */
    if(position_output != NULL) *position_output = position;
    if(cripto_salt_output != NULL) strcpy(cripto_salt_output, cripto_salt);

    return 1; // usuario autorizado
}

int hasPermissionByBase64(int authFd, char *authBase64) {
    char user[127], password[127];
    char *decodedAuthPassword;
    char *decodedAuth;
    size_t decodedAuthSize;

    /* decodifica autenticacao enviada, formato user:password */
    decodedAuth = base64_decode(authBase64, strlen(authBase64), &decodedAuthSize);
    decodedAuth[decodedAuthSize] = 0;

    /* separa os valores user e password em suas variáveis */
    for(decodedAuthPassword = decodedAuth; (*decodedAuthPassword != ':') &&
                                           (*decodedAuthPassword != 0); decodedAuthPassword++);
    if(*decodedAuthPassword == 0) return 0;
    *decodedAuthPassword = 0;
    decodedAuthPassword++;
    strcpy(user, decodedAuth);
    strcpy(password, decodedAuthPassword);
    free(decodedAuth);

    return hasPermission(authFd, user, password, NULL, NULL);
}

void configurePathRelativeToProgram(char* destination, char *programPath, char *prefix) {
    int i, j;

    /* Isola o caminho utilizado para rodar o comando */
    strcpy(destination, programPath);
    for(i = 0; destination[i] != 0; i++);
    for(; i>=0 && destination[i] != '/'; i--);
    
    if(i == 0) {
        printf("\nCaminho do programa não identificado, encerrando execução...\n");
        exit(1);
    }

    /* Adiciona prefixo informado. Ex: nome de uma pasta no mesmo diretório do programa */
    for(j = 0; prefix[j] != 0; j++)
        destination[++i] = prefix[j];
    destination[++i] = 0;
}

void configurePasswordFilesPath(char *programPath) {
    configurePathRelativeToProgram(serverPasswordsPath, programPath, "passwords");
}
