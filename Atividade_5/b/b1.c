/* b1.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/* Processo 1 (read) */

int main()
{
int fd, i;
char buf[512];

    if((fd = open("teste.b", O_EXCL|O_CREAT|O_RDONLY, 0600)) == -1)
    {
        if((fd = open("teste.b", O_RDONLY, 0600)) == -1)
        {
            printf("Erro na abertura de arquivo\n");
            exit(errno);
        }
    }
    if((i = read(fd, buf, sizeof(buf))) == -1)
    {
        printf("Erro na leitura de arquivo\n");
        exit(errno);
    }
    printf("1a. Leitura: Foram lidos %d bytes.\n", i);
    for (i=0; i<sizeof(buf); i++)
        printf("%c", buf[i]);
    printf("\n");
    sleep(5);
    if((i = read(fd, buf, sizeof(buf))) == -1)
    {
        printf("Erro na leitura de arquivo\n");
        exit(errno);
    }
    printf("2a. Leitura: Foram lidos %d bytes.\n", i);
    for (i=0; i<sizeof(buf); i++)
        printf("%c", buf[i]);
    printf("\n");
    close(fd);
    exit(0);
}
