#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "./dijkstra.h"

// quantidade de leitores lendo
int readcount = 0;

//
// TODO: Definição dos semáforos (variaveis precisam ser globais)
int marea;
int mcont;
//

// dado compartilhado que os leitores e escritores acessarão
int shared = 0;

// prototipos das funcoes
void * leitor(void *);
void * escritor(void *);
int gera_rand(int);

int main(int argc, char ** argv){
    // threads dos leitores
    pthread_t * tl;

    // threads dos escritores
    pthread_t * te;

    int i;

    srand(time(NULL));

    if ( argc < 3 )
    {
        printf("Usage: %s num_leitores num_escritores\n", argv[0]);
        return 0;
    }

    //
    // TODO: Criação dos semáforos (aqui é quando define seus
    // valores, usando a biblioteca dijkstra.h
        marea = sem_create(123, 1);
        mcont = sem_create(456, 1);
    // 
 
    // num leitores
    int N_LEITORES = atoi(argv[1]);
    
    // num escritores
    int N_ESCRITORES = atoi(argv[2]);
    
    // gerando uma lista de threads de leitores
    tl = malloc(N_LEITORES * sizeof(pthread_t));

    // iniciando as threads dos leitores
    for (i = 0; i < N_LEITORES; i++)
    {
        pthread_create(&tl[i], NULL, leitor, ( void *)&i);
    }
    
    // gerando uma lista de threads de leitores
    te = malloc(N_ESCRITORES * sizeof(pthread_t));

    // iniciando as threads dos escritores
    for (i = 0; i < N_ESCRITORES; i++)
    {
        pthread_create(&te[i], NULL, escritor, ( void *)&i);
    }

    // aguardando as threads leitoras terminarem
    for (i = 0; i < N_LEITORES; i++)
    {
        pthread_join(tl[i], NULL);
    }
    
    // aguardando as threads escritoras terminarem
    for (i = 0; i < N_ESCRITORES; i++)
    {
        pthread_join(te[i], NULL);
    }

    //
    // TODO: Excluindo os semaforos (dijkstra.h)
        sem_delete(marea);
        sem_delete(mcont);
    // 

    // liberando a memoria alocada
    free(tl);
    free(te);

    return 0;
}

void * leitor(void * id){
    usleep(gera_rand(1000000));

    // convertendo o Id do leitor para int
    int i = (intptr_t) id;

    //
    // TODO: precisa fazer o controle de acesso à entrada do leitor
    // Da o wait no semaforo de contador, incrementa a quantidade de leitores que estão lendo e, se for igual a 1, da um wait no semaforo de acesso a area da seção crítica, compartilhada com os escritores, por fim, liberando o semaforo de contador
    Wait(mcont);
    readcount++;
    if (readcount == 1){
        Wait(marea);
    }
    Signal(mcont);
    //
    
    printf("> Leitor %d tentando acesso\n",i);

        // leitor acessando o valor de shared
        printf("\t> Leitor %d acessando shared: %d - readcount: %d\n",
                i,shared,readcount);

    //
    // TODO: precisa fazer a saída do leitor e liberação do acesso
    // Da o wait no semaforo de contador, decrementa a quantidade de leitores que estão lendo e, se for igual a 0, libera o semaforo de acesso a area da seção crítica, ja que não tem mais nenhum leitor acessando ela, por fim, liberando o semaforo de contador
    Wait(mcont);
    readcount--;
    if (readcount == 0){
        Signal(marea);
    }
    Signal(mcont);
    //

    printf("< Leitor %d liberando acesso\n",i);
}

void * escritor(void * id){
    usleep(gera_rand(1000000));
    
    // convertendo o Id do leitor para int
     int i = (intptr_t) id;

    //
    // TODO: precisa controlar o acesso do escritor ao recurso
    // Da um wait no semáforo da seção crítica, compartilhada com os leitores. Ja que o escritor precisa de acesso restrito a área.
    Wait(marea);
    //
    
    printf("+ Escritor %d tentando acesso\n",i);
    
    printf("\t+ Escritor %d conseguiu acesso\n",i);
    int rnd = gera_rand(100);
    printf("\t+ Escritor %d gravando o valor %d em shared\n", i, rnd);
    usleep(gera_rand(1000000));
    shared = rnd;
    
    //
    // TODO: precisa fazer a saída do escritor e liberação do acesso
    // Libera o semáforo da seção crítica, permitindo a entrada de outro escritor ou de leitores a seção crítica.
    Signal(marea);
    //
    
    printf("+ Escritor %d saindo\n",i);
}

int gera_rand(int limit){
    // 0 a (limit -1)
    return rand()%limit;
}