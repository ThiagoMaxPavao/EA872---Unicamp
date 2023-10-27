/* a.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main()
{
int fd1, fd2, i;
char buf[30];

    if ((fd1 = creat("teste.a", 0600)) == -1)
        printf("Erro na 1a. operacao = %d\n", errno);
    if ((i = write (fd1, "Este e' o programa de teste 1", 29)) == -1)
        printf("Erro na 2a. operacao = %d\n", errno);
    else
        printf("Foram escritos %d bytes.\n", i);
    if (read(fd1, buf, sizeof(buf)) == -1)
        printf("Erro na 3a. operacao = %d\n", errno);
    if ((fd2 = open("teste.a", O_RDONLY, 0600)) == -1)
        printf("Erro na 4a. operacao = %d\n", errno);
    if ((i = read(fd2, buf, sizeof(buf))) == -1) 
        printf("Erro na 5a. operacao = %d\n", errno);
    else
        printf("Foram lidos %d bytes.\n", i);
    if (write(fd2, "/0", 1) == -1)
        printf("Erro na 6a. operacao = %d\n", errno);
    close(fd1);
    close(fd2);
    exit(0);
}
