#include <stdio.h>
#include <sys/signal.h>

void ger_nova();

main()
{
void (*ger_antiga)();

	/* ctrl-c == sinal SIGINT */
	printf ("Primeiro trecho de tratamento de ctrl-c \n");
	signal(SIGINT, SIG_IGN);
	sleep(5);
	printf("\n");
	printf ("Segundo trecho de tratamento de ctrl-c \n");
	signal (SIGINT, ger_nova);
	sleep(5);
	printf("\n");
	printf ("Terceiro trecho de tratamento de ctrl-c \n");
	ger_antiga = (void *) signal(SIGINT, SIG_DFL);
	sleep(5);
	printf("\n");
	printf ("Quarto trecho de tratamento de ctrl-c \n");
	signal(SIGINT, ger_antiga);
	sleep(5);
	printf ("\nTchau!\n");
}

void ger_nova()
{
	printf ("O sinal SIGINT foi captado. Continue a execucao!\n");
}

