/* d.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
int i, j, k;
char buf[50];
 
    if((i = open(argv[1], O_RDONLY, 0600)) == -1)
    {
        printf("Erro na abertura de arquivo\n");
        exit(errno);
    }

    j = dup(i);

    read(i, buf, sizeof(buf));
    printf("1a. Leitura:\n");
    for (k=0; k<50; k++)
        printf("%c",buf[k]);
    printf("\n");

    read(j, buf, sizeof(buf));
    printf("2a. Leitura:\n");
    for (k=0; k<50; k++)
        printf("%c",buf[k]);
    printf("\n");
    close(i);

    read(j, buf, sizeof(buf));
    printf("3a. Leitura:\n");
    for (k=0; k<50; k++)
        printf("%c",buf[k]);
    printf("\n");
    close(j);
    exit(0);
}

