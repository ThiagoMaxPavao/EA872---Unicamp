/* f.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fd, i;
    char buf[50];
    struct stat statinfo;
 
    if (argc != 2)
    {
        printf("Uso: comando <nome de arquivo>\n");
        exit(-1);
    }

    link(argv[1], "tmp");

    if ((fd = open("tmp", O_RDONLY, 0600)) == -1)
    {
        printf("Falha na abertura de arquivo\n");
        exit(-1);
    }

    if (unlink("tmp") == -1)
        exit(-1);

    if (stat("tmp", &statinfo) == -1)
        printf("Falha na chamada <stat>!\n");
    else
        printf("A chamada <stat> foi bem sucedida!\n");

    if (fstat(fd, &statinfo) == -1)
        printf("Falha na chamada <fstat>!\n");
    else
        printf("A chamada <fstat> foi bem sucedida!\n");

    i = read(fd, buf, sizeof(buf));
    printf ("Leu %d bytes: %s\n", i,buf);

    exit(1);
}

