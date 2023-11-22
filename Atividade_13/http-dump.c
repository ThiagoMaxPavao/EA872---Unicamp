/*
 * http-dump: permite inspecionar comandos http enviados a um servidor web por um navegador qualquer
 *
 * Profs. Eleri Cardozo e Marco A. A. Henriques
 *
 */

/*
 * Arquivos a incluir com cabeçalhos e definições
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


/*
 * Programa principal
 */

int main(arg_cont, arg_valor)
int arg_cont;
char **arg_valor;
{
    unsigned short porta;		/* porta a se conectar no servidor		*/
    char area[1024];			/* area para envio e recebimento de dados	*/
    struct sockaddr_in cliente;		/* estrutura de informações sobre os clientes	*/
    struct sockaddr_in servidor;	/* estrutura de informações sobre o servidor	*/
    int soquete;			/* soquete para aceitação de pedidos de conexão	*/
    int novo_soquete;			/* soquete de conexão aos clientes		*/
    unsigned int nome_compr;		/* comprimento do nome de um cliente		*/
    unsigned int mensagem_compr;	/* comprimento da mensagem recebida		*/
    int i;
 

    /*
     * Verifique os argumentos. Deve haver apenas um: o número da porta a se conectar.
     */

    if (arg_cont != 2)
    {
        fprintf(stderr, "Uso: %s número_da_porta\n", arg_valor[0]);
        exit(1);
    }

    /*
     * Primeiro argumento deve ser o número da porta.
     */
    porta = (unsigned short) atoi(arg_valor[1]);

    /*
     * Crie um soquete para aceitar pedidos de conexão.
     */
    if ((soquete = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro em socket()");
        exit(2);
    }

    /*
     * Ligue o soquete ao endereço do servidor.
     */
    servidor.sin_family 	= AF_INET;
    servidor.sin_port   	= htons(porta);
    servidor.sin_addr.s_addr	= INADDR_ANY;

    if (bind(soquete, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
    {
       perror("Erro em bind().\n");
       exit(3);
    }

    /*
     * Aguarde por pedidos de conexão com um fila de até 5 pedidos.
     */
    if (listen(soquete, 5) != 0)
    {
        perror("Erro em listen().\n");
        exit(4);
    }

    /*
     *  Entre em laço infinito
     */
    printf("\n%s já está aceitando conexões de clientes HTTP.\n", arg_valor[0]);
    fflush(stdout);

    while (1) {

    /*
     * Aceite um pedido de conexão.
     */
    nome_compr = sizeof(cliente);
    if ((novo_soquete = accept(soquete, (struct sockaddr *)&cliente, &nome_compr)) == -1)
    {
        perror("Erro em accept().\n");
        break;
    }

    /*
     * Receba a mensagem no novo soquete.
     */
    if ((mensagem_compr = recv(novo_soquete, area, sizeof(area), 0)) == -1)
    {
        perror("Erro em recv().\n");
        break;
    }

    
    /* 
     * Imprima o que recebeu e feche a conexão.
     */
    printf("\nMensagem recebida:\n");
    for(i = 0; i < mensagem_compr; i++) printf("%c", area[i]);
    fflush(stdout);
    shutdown(novo_soquete,2);

    }  /* Laço termina aqui */

    shutdown(soquete,2); 
    printf("O servidor terminou com erro.\n");
    exit(5);
}
