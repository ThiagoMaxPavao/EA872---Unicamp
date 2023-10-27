/* h.c
   Redirecionamento de entrada/saida de dados 
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	int fd1, fd2;

	/* fecha stdin - proxima chamada open tera' descritor igual a 0 */
	close(0); 

	/* abre arquivo que contem a entrada */
	fd1 = open("h.c", O_RDONLY, 0600);
	if(fd1 == -1) {
	  perror("open fd1");
	  exit(0);
	}

	/* fd1 devera ter valor 0 */
	printf("fd1: %d\n", fd1); fflush(stdout);

	/* fecha stdout - proxima chamada open tera' descritor igual a 1 */
	close(1); 

	/* abre arquivo que contera a saida */
	fd2 = open("./teste.h", O_WRONLY | O_CREAT, 0600);
	if(fd2 == -1) {
	  perror("open fd2");
	  exit(0);
	}


	/* executa grep */
	system("grep \"open\"");
	
	/* fecha arquivos */
	close(fd1);
	close(fd2);
}

