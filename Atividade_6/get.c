#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* 
Concatena as strings path1 e path2 e grava o resultado em destination.
retorna a string destination. Coloca uma barra entre os caminhos,para 
impedir erros em casos que ela não tenha sido fornecida. Vale notar que
como path1 é um diretório, inserir barras extras não causa problema.
*/
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

/* Copia o nome do arquivo especificado por path para a string name */
void getFilename(char* path, char *name) {
    char* aux = strrchr(path, '/') + 1;
    strcpy(name, aux);
}

/*
Abre o arquivo de erro, de acordo com o status fornecido.
Acessa o arquivo error_{status}.html e abre, inserindo o valor no ponteiro fd.
Também copia o nome para filename e retorna o status.
*/
int openAndReturnError(int status, char* webspace, int* fd, char* filename) {
    char path[127];
    switch(status) {
        case 403: strcpy(filename, "error_403.html"); break;
        case 404: strcpy(filename, "error_404.html"); break;
        case 500: strcpy(filename, "error_500.html"); break;
        default: return status; break;
    }
    join_paths(path, webspace, filename);
    *fd = open(path, O_RDONLY); // se não abrir, fd=-1 e não retorna pagina de erro.
    return status;
}

/*
Acessa webspace/resource no sistema de arquivos e abre o arquivo
o file descriptor é armazenado no endereco fd.
Retorna o código de erro, se tiver ocorrido, ou 200 se tudo ocorreu bem. (status)
Também procura um arquivo index.html ou welcome.html se webspace/resource for um diretório.
Retorna o file descriptor desses arquivos nesses casos, se forem encontrados.
Se ocorrer erro, chama openAndReturnError e retorna o status. Caso exista o arquivo
error_{status}.html, ele é aberto e retornado.
*/
int get(char* webspace, char* resource, int* fd, char* filename) {
    char path[127];
    char path_aux1[127];
    char path_aux2[127];
    struct stat statbuf;
    int exist1, exist2;

    *fd = -1; // valor retorno se arquivo não for aberto arquivo nenhum.

    /* Cria o caminho completo do recurso desejado */
    join_paths(path, webspace, resource);

    if(stat(path, &statbuf) == -1) {
        return openAndReturnError(404, webspace, fd, filename); // Recurso não encontrado - 404 Not Found
    }

    if(access(path, R_OK) != 0) {
        return openAndReturnError(403, webspace, fd, filename); // Sem permissão de leitura - 403 Forbidden
    }

    /* Verifica se é um arquivo e abre */
    if((statbuf.st_mode & S_IFMT) == S_IFREG) {
        getFilename(path, filename);
        if((*fd = open(path, O_RDONLY)) == -1)
            return openAndReturnError(500, webspace, fd, filename); // Erro interno no servidor
        return 200;
    }

    /* Confirma que é um diretório */
    if((statbuf.st_mode & S_IFMT) != S_IFDIR) {
        return openAndReturnError(500, webspace, fd, filename); // Erro interno no servidor
    }

    /* Verifica permissão de varredura no diretório (bit X) */
    if(access(path, X_OK) != 0) {
        return openAndReturnError(403, webspace, fd, filename); // Sem permissão de leitura - 403 Forbidden
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
        return openAndReturnError(404, webspace, fd, filename); // Recurso não encontrado - 404 Not Found
    }

    /* Verifica se index.html existe e tem permissão de leitura, se tiver abre e imprime */
    if(exist1 != -1 && access(path_aux1, R_OK) == 0) {
        getFilename(path_aux1, filename);
        if((*fd = open(path_aux1, O_RDONLY)) == -1)
            return openAndReturnError(500, webspace, fd, filename); // Erro interno no servidor
        return 200;
    }

    /* Faz a mesma coisa, mas com welcome.html */
    if(exist2 != -1 && access(path_aux2, R_OK) == 0) {
        getFilename(path_aux2, filename);
        if((*fd = open(path_aux2, O_RDONLY)) == -1)
            return openAndReturnError(500, webspace, fd, filename); // Erro interno no servidor
        return 200;
    }

    /*
        Chegando aqui, com certeza pelo menos um dos arquivos existe
        mas nenhum deles tem acesso de leitura, retorna forbidden.
    */
    return openAndReturnError(403, webspace, fd, filename); // Sem permissão de leitura - 403 Forbidden
}
