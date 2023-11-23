#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

static char serverPagesPath[100];

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

    // Verifica se o recurso é protegido
    authStatus = hasAuthentication(webspace, resource, &authFd);

    if(authStatus < 0) {
        return openAndReturnError(500, fd, filename); // Erro interno no servidor
    }
    
    // Verifica se usuário tem autorização
    if(authStatus && (authBase64 == NULL || !hasPermission(authFd, authBase64))) {
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
