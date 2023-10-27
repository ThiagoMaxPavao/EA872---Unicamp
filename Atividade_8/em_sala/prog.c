#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    int soquete;
    struct sockaddr_in destino;
    char requisicao[] = "GET /dir1/dir11/../texto1.html HTTP/1.1\r\nConnection: close\r\nAccept: */*\r\n\r\n";
    char resposta[10000];
    int n_read;
    int fd_index;

    if(argc != 3) {
        printf("Uso: %s <IP do servidor > <Porta>\n", argv[0]);
        exit(1);
    }

    if((fd_index = open("index.html", O_CREAT | O_WRONLY, 0600)) == -1) {
        perror("Erro na abertura do arquivo index.html");
        exit(2);
    }

    /* Inicia socket */
    soquete = socket(AF_INET, SOCK_STREAM, 0);
    destino.sin_family = AF_INET;
    destino.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], (struct in_addr *)&destino.sin_addr.s_addr);

    /* Inicia conexao e detecta erro */
    if(connect(soquete, (struct sockaddr*)&destino, sizeof(destino)) == -1) {
        perror("Erro de conexão com o servidor");
        exit(3);
    }

    /* Envia requisicao GET definida */
    write(soquete, requisicao, strlen(requisicao));

    printf("--- Requisição ---\n%s", requisicao);

    sleep(2);

    /* Lê o primeiro bloco da resposta, contendo todo o Header */
    n_read = read(soquete, resposta, sizeof(resposta)-1);
    resposta[n_read] = 0;

    printf("--- Resposta ---\n%s\n--- Fim ---\n", resposta);
    exit(0);

    /* Encontra o fim do header, pela sequencia CRLFCRLF */
    char* comecoBody = strstr(resposta, "\r\n\r\n") + 4;
    write(1, comecoBody, strlen(comecoBody));
    write(fd_index, comecoBody, strlen(comecoBody));

    /*
        Configura socket como nao bloqueante,
        para finalizar assim que nao houver mais
        dados no buffer de resposta
    */
    fcntl(soquete, F_SETFL, O_NONBLOCK);

    /* Le do buffer e imprime ate nao ter nada para ler */
    while(1) {
        n_read = read(soquete, resposta, sizeof(resposta));
        if(n_read == -1) break;
        write(1, resposta, n_read);
        write(fd_index, resposta, n_read);
    }

    /* Encerra a conexao */
    close(soquete);
}
