#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

main(int argc,char *argv[])
{
int pid;
int estado;
struct timeval tv1, tv2;
double delta;
	pid = fork();
	if (pid == 0)
	{
		printf("pid(processo-filho)=%d\n",getpid());
		execlp("/bin/cal","cal",argv[1],argv[2],NULL);     /* linha 15 */
		perror("Erro ao executar cal");
	}
	else
	{
		gettimeofday(&tv1,NULL);
		wait(&estado);
		gettimeofday(&tv2,NULL);
		printf ("pid(processo-pai)=%d\n",getpid());
		delta = ((double)(tv2.tv_sec) + (double)(tv2.tv_usec)/1e6) -
		        ((double)(tv1.tv_sec) + (double)(tv1.tv_usec)/1e6);
		printf("O tempo de execucao do processo %d e' %lf,", pid, delta);
		printf(" terminando com estado=%d\n", estado>>8);
	}
	printf ("Processo %d terminou!\n", getpid());
	exit(30);
}

