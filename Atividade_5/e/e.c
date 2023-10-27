/* e.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
int fd;
    fd = creat(argv[1], 0600);
    dup2(fd,1);
    close(fd);
    execvp(argv[2], &argv[2]);
}

