#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dijkstra.h"
 
#define KEY 123

int main()
{
  int sem ;
  sem = sem_create(KEY,1) ;
  printf("Um semáforo foi criado com o identificador %d\n",sem) ;

  if (fork() == 0) { 
    printf("\tProcesso filho usa o recurso\n");
    Wait(sem);
    sleep(8) ;
    printf("\tProcesso filho libera o recurso.\n") ;
    Signal(sem) ;
    sleep(1);
  }
  else {
    sleep(1);
    printf("Processo PAI bloqueia ao tentar acessar recurso.\n");
    Wait(sem);
    printf("Recurso disponível para o processo PAI.\n");
    sem_delete(sem);
  }
  exit(0);
    
}



