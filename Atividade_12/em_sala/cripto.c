/* Compile com: gcc -o cripto cripto.c -lcrypt */
#define _XOPEN_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
int main(int argc, char **argv){
	if (argc < 3) exit(0);
	printf("\n( Salt = %s ) + ( Password = %s ) ==> ( crypt = %s )\n\n",
				argv[2], argv[1], crypt(argv[1], argv[2]));
	return(0);
}