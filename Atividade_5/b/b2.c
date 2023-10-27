/* b2.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/* Processo 2 (write) */

int main()
{
int fd, i;
char buf[512];
 
    if((fd = open("teste.b", O_EXCL|O_CREAT|O_WRONLY, 0600)) == -1)
    {
        if((fd = open("teste.b", O_WRONLY, 0600)) == -1)
        {
            printf("Erro na abertura de arquivo\n");
            exit(errno);
        }
    }
    for (i=0; i < sizeof(buf); i++)
        buf[i] = '*';
    if((i = write(fd, buf, sizeof(buf))) == -1)
    {
        printf("Erro na escrita em arquivo: %d\n", errno);
        exit(errno);
    }
    printf("1a. Escrita: Foram escritos %d bytes.\n", i);
    sleep(5);
    if((i = write(fd, buf, sizeof(buf))) == -1)
    {
        printf("Erro na escrita em arquivo: %d\n", errno);
        exit(errno);
    }
    printf("2a. Escrita: Foram escritos %d bytes.\n", i);
    close(fd);
    exit(0);
}

