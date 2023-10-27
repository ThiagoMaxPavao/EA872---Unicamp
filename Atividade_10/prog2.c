#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define MAX_THREADS     10


void *WorkerThread(void *t)
{
   int i, tid;
   double result=0.0;
   tid = (intptr_t)t;

   printf("Comecando thread %d ...\n",tid);

   for (i=0; i<1000000; i++)
   {
     result = result + (double)random();
   }

   printf("Thread %d terminada. Resultado = %e\n",tid, result);
   pthread_exit((void*)(intptr_t)t);
}

int main(int argc, char *argv[])
{
   pthread_t thread[MAX_THREADS];
   pthread_attr_t attr;
   int n, rc, t, k;
   void *status;

   if(argc != 2){
	printf("Uso: %s <num_threads>\n", argv[0]);
	exit(1);
   }

   n = atoi(argv[1]);
   if(n > MAX_THREADS || n <= 0) n = MAX_THREADS;

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   for(t = 0; t < n; t++) {
      printf("Main: criando thread %d\n", t);
      rc = pthread_create(&thread[t], &attr, WorkerThread, (void *)(intptr_t)t);
      if (rc) {
         printf("\n ERRO: codigo de retorno de pthread_create eh %d \n", rc);
         exit(1);
      }
   }

   pthread_attr_destroy(&attr);
   for(t = 0; t < n ; t++) {
      rc = pthread_join(thread[t], &status);
      if (rc) {
         printf("\n ERRO: codigo de retorno de pthread_join eh %d \n", rc);
         exit(1);
      }
      k = (intptr_t)status;
      printf("Main: completou join com thread %d com status %d\n", t, k);
   }

   printf("Programa %s terminado\n", argv[0]);
   pthread_exit(NULL);
}
