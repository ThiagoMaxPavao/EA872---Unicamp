/* c.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main()
{
int fd, i;

    if((fd = open("testec1.txt", O_CREAT|O_WRONLY|O_TRUNC, 0600)) == -1)
    {
        printf("Erro na abertura de arquivo\n");
        exit(errno);
    }
    write (fd, "Este e' o arquivo de teste c1: ", 31);
    lseek (fd, 20031, SEEK_SET);  /*  consulte man !  */
    write (fd, " fim do arquivo testec1.txt\n", 28);
    close (fd);

    fd = creat("testec2.txt", 0600);
    write (fd, "Este e' o arquivo de teste c2: ", 31);
    for (i=0; i<20000; i++)
        write(fd,"$",1);
    write (fd, " fim do arquivo testec2.txt\n", 28);
    close (fd);
    exit(0);
}
