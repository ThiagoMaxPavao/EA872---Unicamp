/* Programa teste_select.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
    int n, c;
    // long int tolerancia = 5;
    fd_set fds;
    struct timeval timeout;
    int fd;

    fd = 0;              /* O que faz esta linha e as demais? Comente cada uma. */
    FD_ZERO(&fds);       /*  */
    FD_SET(fd, &fds);    /*  */
    timeout.tv_sec = 5;  /*  */
    timeout.tv_usec = 0; /*  */

    n = select(1, &fds, (fd_set *)0, (fd_set *)0, &timeout);
    /* O que faz a linha acima? */

    if(n > 0 && FD_ISSET(fd, &fds))   /*  */
    {
        c = getchar();  /* O que acontecerá de diferente aqui? */
        printf("Caractere %c teclado. Tempo restante: %lf\n", c,
        (double)timeout.tv_sec + ((double)timeout.tv_usec / 1000000.0));
    }
    else if(n == 0)    /*  */
    {
        printf("Nada foi teclado em 5 s.\n");
    }
    else perror("Erro em select():");   /*  */
    exit(1);   /*  */
}
