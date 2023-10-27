
#include <stdio.h>
#include <stdlib.h>

main()
{
int pid1, pid2, filho1, filho2;
int estado1, estado2;
	printf("pid(processo)=%d, pid(processo-pai)=%d\n",getpid(),getppid());
	pid1 = fork();
	if (pid1 == 0)
	{
		printf("Primeiro processo-filho:\n");
		printf("pid(1o. processo-filho)=%d, pid(processo-pai)=%d\n",getpid(),getppid());
		sleep(10);
		exit(20);
	}
	else
	{
		pid2 = fork();
		if (pid2 == 0)
		{
			printf("Segundo processo-filho:\n");
			printf("pid(2o. proc-filho)=%d, pid(processo-pai)=%d\n",getpid(),getppid());
		}
		else
		{
			filho1 = wait(&estado1);
			filho2 = wait(&estado2);
			printf ("Processo-pai:\n");
			printf ("pid(1o. processo-filho)=%d\n",pid1);
			printf ("pid(2o. processo-filho)=%d\n",pid2);
			printf("O processo %d terminou com estado=%d\n",filho1,estado1>>8);
			printf("O processo %d terminou com estado=%d\n",filho2,estado2>>8);
		}
	}
	printf ("Processo %d terminou!\n",getpid());
	exit(30);
}
