#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static char serverPagesPath[100]; // Caminho para o script, para obter as páginas do servidor
extern char *changePasswordFilename; // Nome para ser reconhecido na URL para a funcionalidade de troca de senha

void configureServerPagesPath(char *programPath) {
    int i, j;
    char folder[] = "server_pages";
    strcpy(serverPagesPath, programPath);
    for(i = 0; serverPagesPath[i] != 0; i++);
    for(; i>=0 && serverPagesPath[i] != '/'; i--);
    
    if(i == 0) {
        printf("\nCaminho do programa não identificado, encerrando execução...\n");
        exit(1);
    }

    for(j = 0; folder[j] != 0; j++)
        serverPagesPath[++i] = folder[j];
    serverPagesPath[++i] = 0;
}

int openAndReturnError(int status, int* fd, char* filename) {
    char path[127];
    switch(status) {
        case 403: strcpy(filename, "error_403.html"); break;
        case 404: strcpy(filename, "error_404.html"); break;
        case 405: strcpy(filename, "error_405.html"); break;
        case 500: strcpy(filename, "error_500.html"); break;
        case 503: strcpy(filename, "error_503.html"); break;
        default: return status; break;
    }
    join_paths(path, serverPagesPath, filename);
    *fd = open(path, O_RDONLY); // se não abrir, fd=-1 e não retorna pagina de erro.
    return status;
}

int get(char* webspace, char* resource, int* fd, char* filename, char* authBase64) {
    char path[127];
    char path_aux1[127];
    char path_aux2[127];
    struct stat statbuf;
    int authStatus;
    int exist1, exist2;
    int authFd = -1; // arquivo de senhas para o recurso especificado

    *fd = -1; // valor retorno se arquivo não for aberto arquivo nenhum.

    if(!isPathConfined(resource)) {
        return openAndReturnError(403, fd, filename); // Fora do webspace - 403 Forbidden
    }
    
    if(stringEndsWith(resource, ".htaccess")) {
        return openAndReturnError(403, fd, filename); // Tentativa de acessso ao .htaccess - 403 Forbidden
    }

    // Verifica se o recurso é protegido
    authStatus = hasAuthentication(webspace, resource, &authFd);

    // Erro ao abrir .htaccess ou .htpassword
    if(authStatus < 0) {
        printf("Ocorreu um erro ao executar hasAuthentication,\nPossivelmente na abertura de .htaccess ou .htpassword.\n");
        if(authFd != -1) close(authFd);
        return openAndReturnError(500, fd, filename); // Erro interno no servidor
    }

    /*
    Verifica se o recurso solicitado é o formulário de troca de senha
    Só verifica se o recurso for protegido.
    */
    if(authStatus && stringEndsWith(resource, changePasswordFilename)) {
        join_paths(path, serverPagesPath, "change_password.html");
        getFilename(path, filename);
        if((*fd = open(path, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 200;
    }
    
    // Verifica se usuário tem autorização
    if(authStatus && (authBase64 == NULL || !hasPermissionByBase64(authFd, authBase64))) {
        // Fecha o arquivo de senhas aberto por hasAuthentication
        if(authFd != -1) close(authFd);

        // Autenticação não informada ou informada mas não validada.
        return openAndReturnError(401, fd, filename); // Autenticação necessária
    }

    // Fecha o arquivo de senhas aberto por hasAuthentication
    if(authFd != -1) close(authFd);

    /* Cria o caminho completo do recurso desejado */
    join_paths(path, webspace, resource);

    if(stat(path, &statbuf) == -1) {
        if(errno == EACCES) // Algum diretório sem permissão de execução
            return openAndReturnError(403, fd, filename);
        return openAndReturnError(404, fd, filename); // Recurso não encontrado - 404 Not Found
    }

    if(access(path, R_OK) != 0) {
        return openAndReturnError(403, fd, filename); // Sem permissão de leitura - 403 Forbidden
    }

    /* Verifica se é um arquivo e abre */
    if((statbuf.st_mode & S_IFMT) == S_IFREG) {
        getFilename(path, filename);
        if((*fd = open(path, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 200;
    }

    /* Confirma que é um diretório */
    if((statbuf.st_mode & S_IFMT) != S_IFDIR) {
        return openAndReturnError(500, fd, filename); // Erro interno no servidor
    }

    /* Verifica permissão de varredura no diretório (bit X) */
    if(access(path, X_OK) != 0) {
        return openAndReturnError(403, fd, filename); // Sem permissão de leitura - 403 Forbidden
    }

    /*
        Cria o caminho completo para index.html e welcome.html
        no diretório especificado na requisição
    */
    join_paths(path_aux1, path, "/index.html");
    join_paths(path_aux2, path, "/welcome.html");

    /* Verifica se nenhum dos arquivos existe */
    if((( exist1 = stat(path_aux1, &statbuf) ) == -1) && 
       (( exist2 = stat(path_aux2, &statbuf) ) == -1)) {
        return openAndReturnError(404, fd, filename); // Recurso não encontrado - 404 Not Found
    }

    /* Verifica se index.html existe e tem permissão de leitura, se tiver abre e imprime */
    if(exist1 != -1 && access(path_aux1, R_OK) == 0) {
        getFilename(path_aux1, filename);
        if((*fd = open(path_aux1, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 200;
    }

    /* Faz a mesma coisa, mas com welcome.html */
    if(exist2 != -1 && access(path_aux2, R_OK) == 0) {
        getFilename(path_aux2, filename);
        if((*fd = open(path_aux2, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 200;
    }

    /*
        Chegando aqui, com certeza pelo menos um dos arquivos existe
        mas nenhum deles tem acesso de leitura, retorna forbidden.
    */
    return openAndReturnError(403, fd, filename); // Sem permissão de leitura - 403 Forbidden
}

int processPost(char* webspace, char* resource, int* fd, char* filename, char *requestContent) {
    char path[127];
    char username[127] = "";
    char password[127] = "";
    char newPassword[127] = "";
    char newPasswordConfirm[127] = "";
    char content[512];
    const char token_separators[3] = "&=";
    char authBuffer[200];
    char cripto_salt[127];
    char *newPasswordCripto;
    int keepReading = 1;
    int authStatus;
    int authFd = -1;
    int position = 0;
    char *token, c;
    int i;
    int cifraoCount = 0;

    if(!stringEndsWith(resource, changePasswordFilename)) {
        return openAndReturnError(405, fd, filename); // POST sem ser para troca de senha
                                                      // retorn erro 405 - Method Not Allowed
    }
    
    // Verifica se o recurso é protegido
    authStatus = hasAuthentication(webspace, resource, &authFd);

    // Erro na abertura de .htaccess ou .htpassword
    if(authStatus < 0) {
        printf("Ocorreu um erro ao executar hasAuthentication,\nPossivelmente na abertura de .htaccess ou .htpassword.\n");
        if(authFd != -1) close(authFd);
        return openAndReturnError(500, fd, filename); // Erro interno no servidor
    }

    // Diretório sem proteção -> Bad Request, não faz sentido alterar a senha.
    if(authStatus == 0) {
        if(authFd != -1) close(authFd);
        join_paths(path, serverPagesPath, "error_cp_unprotected_directory.html");
        getFilename(path, filename);
        if((*fd = open(path, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 400;
    }

    strcpy(content, requestContent);

    token = strtok(content, token_separators);

    /*
    Percorre cada par chave=valor, reconhece o tipo e salva na variavel correspondente
    Troca os '+' por espaços nas variáveis finais.
    */
    while( token != NULL ) {
        char *currentField = NULL;
        if(strcmp(token, "username") == 0)              currentField = username;
        if(strcmp(token, "current-password") == 0)      currentField = password;
        if(strcmp(token, "new-password") == 0)          currentField = newPassword;
        if(strcmp(token, "new-password-confirm") == 0)  currentField = newPasswordConfirm;

        if(currentField != NULL) {
            token = strtok(NULL, token_separators);
            strcpy(currentField, token);
            for(int i = 0; currentField[i] != 0; i++)
                if(currentField[i] == '+') currentField[i] = ' ';
        }

        token = strtok(NULL, token_separators);
    }

    /*
    Verifica se algum dos campos não foi reconhecido nos pares chave=valor.
    */
    if(strlen(username) == 0 || 
       strlen(password) == 0 ||
       strlen(newPassword) == 0 ||
       strlen(newPasswordConfirm) == 0) {
        if(authFd != -1) close(authFd);
        join_paths(path, serverPagesPath, "error_cp_incomplete_content.html");
        getFilename(path, filename);
        if((*fd = open(path, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 400;
    }

    /*
    Verifica se as duas nova senha coincidem
    */
    if(strcmp(newPassword, newPasswordConfirm) != 0) {
        if(authFd != -1) close(authFd);
        join_paths(path, serverPagesPath, "error_cp_different_new_passwords.html");
        getFilename(path, filename);
        if((*fd = open(path, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 400;
    }

    /*
    Verifica se o usuário e senha bate com algum dos armazenados no arquivo htpassword aberto
    */
    if(!hasPermission(authFd, username, password)) {
        close(authFd);
        join_paths(path, serverPagesPath, "error_cp_unauthorized.html");
        getFilename(path, filename);
        if((*fd = open(path, O_RDONLY)) == -1)
            return openAndReturnError(500, fd, filename); // Erro interno no servidor
        return 400;
    }

    lseek(authFd, 0, SEEK_SET);

    getLine(0, NULL, 1); // reseta o buffer auxiliar da função

    /*
    Encontra linha do usuário no arquivo
    */
    while(keepReading) {
        keepReading = getLine(authFd, authBuffer, 0);
        if(strncmp(username, authBuffer, strlen(username)) == 0) break;
        position += strlen(authBuffer) + 1;
    }

    char *passwordAuth = authBuffer + strlen(username) + 1;

    // Copia o salt da senha
    for(i = 0; cifraoCount < 3 && ((c = passwordAuth[i]) != 0); i++) {
        cripto_salt[i] = c;
        if(c == '$') cifraoCount++;
    }
    cripto_salt[i] = 0;

    // Caso não tenham '$'s na senha, então foi utilizada criptografia default
    // Neste caso copia o salt, que são os dois primeiros caracteres da senha criptografada
    if(cifraoCount == 0) {
        cripto_salt[0] = passwordAuth[0];
        cripto_salt[1] = passwordAuth[1];
        cripto_salt[2] = 0;
    }

    /*
    Gera a nova senha
    */
    newPasswordCripto = crypt(newPassword, cripto_salt);

    /*
    Sobreescreve a antiga no arquivo
    */
    lseek(authFd, position + strlen(username) + 1, SEEK_SET);
    write(authFd, newPasswordCripto, strlen(newPasswordCripto));

    /*
    Retorna página de sucesso
    */
    close(authFd);
    join_paths(path, serverPagesPath, "change_password_success.html");
    getFilename(path, filename);
    if((*fd = open(path, O_RDONLY)) == -1)
        return openAndReturnError(500, fd, filename); // Erro interno no servidor
    return 200; // OK
}
