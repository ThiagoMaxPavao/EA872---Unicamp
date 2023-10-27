#include <stdio.h>
#include <sys/wait.h>
#include <sys/signal.h>

main(){
int pid1, pid2,i;
long j;
void ger_sinal();

       signal(SIGCHLD,ger_sinal);
       pid1 = fork();
       if (pid1 == 0){
               while (1){
                    printf("- Processo %d está ativo/em execução\n",getpid());
                    sleep(1);
                    printf("1\n");
                    fflush(0);
                    for(j=0;j<1000000;j++);
               }
       }
       pid2 = fork();
       if (pid2 == 0){
               while (1){
                    printf("- Processo %d está ativo/em execução\n",getpid());
                    sleep(1);
                    printf("2\n");
                    fflush(0);
                    for(j=0;j<1000000;j++);
               }
       }
       sleep(5);
       printf("%d: Vou parar o 1o.\n",getpid());
       fflush(0);
       kill (pid1,SIGSTOP);
       sleep(5);
       printf("%d: Vou parar o 2o.\n",getpid());
       fflush(0);
       kill (pid2,SIGSTOP);
       sleep(5);
       printf("%d: Vou continuar o 1o.\n",getpid());
       fflush(0);
       kill (pid1,SIGCONT);
       sleep(5);
       printf("%d: Vou continuar o 2o.\n",getpid());
       fflush(0);
       kill (pid2,SIGCONT);
       sleep(5);
       printf("%d: Vou matar o 1o.\n",getpid());
       fflush(0);
       kill (pid1,SIGINT);
       sleep(5);
       printf("%d: Vou matar o 2o.\n",getpid());
       fflush(0);
       kill (pid2,SIGINT);

      /* Sinais s�o ass�ncronos,  logo, n�o se sabe quando v�o "chegar". Se o programa acabar logo ap�s essas chamadas kill(pidX,SIGINT), os sinais v\o chegar tarde demais (o programa j ter finalizado) e n�o ser�o capturados!
       */ 
                                                                                                                                                                                    
       for(i = 0; i < 5; i++){
            printf("%d: Esperando sinal \"chegar\"...\n",getpid());
            fflush(0);
            for(j=0;j<1000000;j++);
            sleep(1);
       }
}

void ger_sinal(){

  int pid;
  int estado;

 /* Este loop � um detalhe importante. Um sinal SIGCHLD (em sistemas BSD) pode estar "representando" m�ltiplos filhos e ent�o o sinal SIGCHLD significa que um ou mais filhos tiveram seu estado alterado.
  */

  do{
    pid = wait3(&estado,WNOHANG,NULL);
    printf("%d: Um sinal de mudança do estado do processo-filho %d foi captado!\n", getpid(),pid);
    if (pid > 0){
       printf("O processo-filho %d foi extinto! Estado=%d\n", pid,estado>>8);
    }
  } while( pid > 0 && printf("em loop!  "));

  signal(SIGCHLD,ger_sinal);
}

