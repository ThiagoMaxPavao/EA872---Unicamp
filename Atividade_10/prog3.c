#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#define MAXITENS 20

/* variaveis globais */
int buffer[MAXITENS];
int pointer = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

/* gera um n´umero aleatorio de itens para produzir / consumir */
int nitens() {
  sleep(1);
/*  return(rand() % MAXITENS); <== esta linha deve ser corrigida 
    como mostrado abaixo.                                         */
  return(rand() % MAXITENS + 1); /* correcao feita para evitar 
                                    que seja retornado o valor 0. */
}

/* insere item */
void insere_item(int item) {
  buffer[pointer] = item;
  printf("\nitem %d inserido (valor=%d)", pointer, item);
  fflush(stdout);
  pointer++;
  sleep(1);
}

/* remove item */
int remove_item() {
  pointer--;
  printf("\nitem %d removido (valor=%d)", pointer, buffer[pointer]);
  fflush(stdout);
  sleep(1);
}

/* produtor */
void* produtor(void* in) {
  int i, k;
  printf("\nProdutor iniciando...");
  fflush(stdout);
  while(1) {
    pthread_mutex_lock(&mutex);
    /* produz */
    k = nitens();
    printf("\nNº de itens a tratar no produtor =%d\n",k);
    for(i = 0; i < k; i++) {
      if(pointer == MAXITENS) { /* buffer cheio -- aguarda */
        printf("\nBuffer cheio -- produtor aguardando..."); fflush(stdout);
        pthread_cond_wait(&cond1, &mutex);
        printf("\nProdutor reiniciando..."); fflush(stdout);
      }
      insere_item(rand() % 100);
    }
    pthread_cond_signal(&cond2);
    pthread_mutex_unlock(&mutex);
    sleep(1);
  }
}

/* consumidor */
void* consumidor(void* in) {
  int i, k;
  printf("\nConsumidor iniciando...");
  fflush(stdout);
  while(1) {
    pthread_mutex_lock(&mutex);
    /* consome */
    k = nitens();
    printf("\nNº de itens a tratar no consumidor =%d\n",k);
    for(i = 0; i < k; i++) {
      if(pointer == 0) { /* buffer vazio */
        printf("\nBuffer vazio -- consumidor aguardando..."); fflush(stdout);
        pthread_cond_wait(&cond2, &mutex);
        printf("\nConsumidor reiniciando..."); fflush(stdout);
      }
      remove_item();
    }
    pthread_cond_signal(&cond1);
    pthread_mutex_unlock(&mutex);
    sleep(1);
  }
}

int main() {
  pthread_t thread1, thread2;
  pthread_create(&thread2, NULL, &consumidor, NULL);
  pthread_create(&thread1, NULL, &produtor, NULL);
  pthread_join( thread1, NULL);
  exit(0);
}
