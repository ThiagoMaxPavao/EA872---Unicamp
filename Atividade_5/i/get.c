#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* 
Concatena as strings path1 e path2 e grava o resultado em destination.
retorna a string destination.
*/
char* join_paths(char* destination, char* path1, char* path2) {
    char c;
    int i = 0;

    /* copia path 1 em destination */
    while((c = path1[i]) != 0) destination[i++] = c;

    /* 
        Adiciona barra entre caminhos, para impedir erros
        em casos que ela não tenha sido fornecida.
        Vale notar que como path1 é um diretório,
        inserir barras extras não causa problema.
    */
    destination[i++] = '/';

    /* Concatena path2 */
    for(int j = 0; (c = path2[j]) != 0; j++) destination[i++] = c;
    destination[i] = 0; // finalizador de string

    return destination;
}

/*
Imprime o arquivo referenciado por fd na tela, seguido por uma quebra de linha
Retorna -1 se houver erro na leitura do arquivo, 0 se não houver erro
*/
int print(int fd) {
    char buffer[50];
    int n; // n -> numero de bytes lidos por read
    while((n = read(fd, buffer, sizeof(buffer))) > 0) {
        write(1, buffer, n); // imprime em stdout
    }
    write(1, "\n", 1);
    return n; // 0 se não houver mais nada para ler, -1 se houve erro na leitura
}

/*
Abre o arquivo no diretório path e imprime.
retorna o código de status, 500 se houve erro e 0 se tudo foi ok
*/
int openAndPrint(char *path) {
    int fd;

    /* Abre o arquivo em modo leitura */
    if((fd = open(path, O_RDONLY)) == -1) {
        return 500; // Erro interno no servidor
    }

    /* Imprime os conteúdos em stdout */
    if(print(fd) == -1) {
        return 500; // Erro interno no servidor
    }
    
    return 0; // OK
}

int get(char* webspace, char* resource) {
    char path[127];
    char path_aux1[127];
    char path_aux2[127];
    struct stat statbuf;
    int exist1, exist2;

    /* Cria o caminho completo do recurso desejado */
    join_paths(path, webspace, resource);

    if(stat(path, &statbuf) == -1) {
        return 404; // Recurso não encontrado - 404 Not Found
    }

    if(access(path, R_OK) != 0) {
        return 403; // Sem permissão de leitura - 403 Forbidden
    }

    /* Verifica se é um arquivo e imprime */
    if((statbuf.st_mode & S_IFMT) == S_IFREG) {
        return openAndPrint(path);
    }

    /* Confirma que é um diretório */
    if((statbuf.st_mode & S_IFMT) != S_IFDIR) {
        return 500; // Erro interno no servidor
    }

    /* Verifica permissão de varredura no diretório (bit X) */
    if(access(path, X_OK) != 0) {
        return 403; // Sem permissão de leitura - 403 Forbidden
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
        return 404; // Recurso não encontrado - 404 Not Found
    }

    /* Verifica se index.html existe e tem permissão de leitura, se tiver abre e imprime */
    if(exist1 != -1 && access(path_aux1, R_OK) == 0) {
        return openAndPrint(path_aux1);
    }

    /* Faz a mesma coisa, mas com welcome.html */
    if(exist2 != -1 && access(path_aux2, R_OK) == 0) {
        return openAndPrint(path_aux2);
    }

    /*
        Chegando aqui, com certeza pelo menos um dos arquivos existe
        mas nenhum deles tem acesso de leitura, retorna forbidden.
    */
    return 403; // Sem permissão de leitura - 403 Forbidden
}
