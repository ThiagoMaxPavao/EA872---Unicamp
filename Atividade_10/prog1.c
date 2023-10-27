#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#define MAX_THREADS	50

void * PrintHello(void * data)
{
    printf("Thread %ld - Criada na iteracao %ld.\n", pthread_self(),  (intptr_t)data);
    pthread_exit(NULL);
}

int main(int argc, char * argv[])
{
    int rc;
    pthread_t thread_id[MAX_THREADS];
    int i, n;

    if(argc != 2){
	printf("Uso: %s <num_threads>\n", argv[0]);
	exit(1);
    }

    n = atoi(argv[1]);
    if(n > MAX_THREADS || n <= 0) n = MAX_THREADS;

    for(i = 0; i < n; i++)
    {
        rc = pthread_create(&thread_id[i], NULL, PrintHello, (void*)(intptr_t)i);
        if(rc)
        {
             printf("\n ERRO: codigo de retorno de pthread_create eh %d \n", rc);
             exit(1);
        }
        printf("\n Thread %ld. Criei a thread %ld na iteracao %d ...\n", 
                pthread_self(), thread_id[i], i);
        //////////////////////////////////////////////
        // if(i % 3 == 0) sleep(2); //<-- linha destacada
        //////////////////////////////////////////////
    }

    pthread_exit(NULL);
}
