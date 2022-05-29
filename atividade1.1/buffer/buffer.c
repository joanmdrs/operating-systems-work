#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "../dijkstra/dijkstra.h"

// 
// TODO: Definição dos semáforos (variaveis precisam ser globais)

int mutex;
int full;
int empty;

// ponteiro para a fila do buffer
int * buffer;

// buffer size
int N_BUFFER = 5; 

// producers num
int PROD_NUM = 5;

// posicao do buffer a gravar
int p = 0;

// posicoes para acessar o buffer
int in = 0;
int out = 0;

// prototipos das funcoes
void *producer(void *);
void *consumer();
int gera_rand(int);
void print_buffer();

int main(int argc, char ** argv)
{
    // thread do consumidor
    pthread_t tcons;

    // threads dos n produtores (vao ser alocadas dinamicamente)
    pthread_t *nprod;

    int i;

    if ( argc != 3 )
    {
        printf("Usage: %s buffer_size num_producers\n", argv[0]);
        return 0;
    }

    // iniciando a semente do random
    srand(time(NULL));

    // buffer size
    N_BUFFER = atoi(argv[1]);

    // num of producers
    PROD_NUM = atoi(argv[2]);

    //
    // TODO: Criação dos semáforos (aqui é quando define seus
    // valores, usando a biblioteca dijkstra.h
    // 
    mutex = sem_create(123, 1);
    full = sem_create(456, N_BUFFER);
    empty = sem_create(789, 0);
    
    // gerando um buffer de N inteiros
    buffer = malloc(N_BUFFER * sizeof(int));

    // "zerando" o buffer com valores -1
    for (i = 0; i < N_BUFFER; i++)
    {
        buffer[i] = -1;
    }

    // gerando uma lista de threads de produtores
    nprod = malloc(PROD_NUM * sizeof(pthread_t));

    // iniciando a thread do consumidor
    pthread_create(&tcons, NULL, consumer, NULL);

    // iniciando as threads dos produtores
    for (i = 0; i < PROD_NUM; i++)
    {
        pthread_create(&nprod[i], NULL, producer,( void *)i);
    }
    
    // iniciando as threads dos produtores
    for (i = 0; i < PROD_NUM; i++)
    {
        pthread_join(nprod[i], NULL);
    }

    // finalizando a thread do consumidor
    pthread_join(tcons, NULL);

    //
    // TODO: Excluindo os semaforos (dijkstra.h)
    // 

    sem_delete(mutex);
    sem_delete(full);
    sem_delete(empty);

    // liberando a memoria alocada
    free(buffer);
    free(nprod);

    return 0;
}

void * consumer()
{
    usleep(gera_rand(1000000));

    // o valor consumido será salvo nesta variável
    int produto;

    int i = 0;

    // consome todos os recursos dos n produtores e finaliza
    while (i != PROD_NUM)
    {
        printf("- Consumidor esperando por recurso!\n");

        print_buffer();

        //
        // TODO: precisa bloquear ate que tenha algo a consumir
        //
        Wait(full);

        printf("- Consumidor entrou em ação!\n");

        print_buffer();

        //
        // TODO: precisa garantir exclusão mutua ao acessar o buffer
        //
        Wait(mutex);


        printf("\t- Consumidor vai limpar posição %d\n", out);

        // acessando o buffer
        produto = buffer[out];
        printf("\t- Consumiu o valor: %d\n",produto);
        // verificando se o consumidor nao esta consumindo sujeira do buffer
        if (buffer[out] == -1)
        {
            printf("==== ALERTA DO CONSUMIDOR ====\nPosicao %d estava vazia\n",in);
        }
        
        // limpando o buffer nesta posição
        buffer[out] = -1;
        out = (out + 1) % N_BUFFER;

        //
        // TODO: saindo da seção critica
        //
        Signal(mutex);

        
        
        //
        // TODO: liberando o recurso
        //
        Signal(empty);

        i++;
    }
}

// recebe o Id de um produtor
void * producer(void * id)
{
    usleep(gera_rand(1000000));

    // recebendo od Id do produtor e convertendo para int
    int i = (int) id;
     
    // valor a ser produzido
    int produto;

    printf("> Produtor %d esperando por recurso!\n",i);

    //
    // TODO: precisa bloquear até que tenha posicao disponível no buffer
    // 

    Wait(empty);
    printf("> Produtor %d entrou em ação!\n",i);

    // 
    // TODO: precisa garantir o acesso exclusivo ao buffer
    //
    Wait(mutex);


    // numero aleatorio de 0 a 99
    produto = gera_rand(100);

    // verificando se o produtor nao esta sobrescrevendo uma posicao
    if (buffer[in] != -1)
    {
        printf("==== ALERTA DO PRODUTOR %d ====\nPosicao %d estava ocupada com o valor %d\n",
                i,in,buffer[in]);
    }

    // gravando no buffer
    printf("\t> Produtor %d vai gravar o valor %d na pos %d\n", i, produto, in);
    buffer[in] = produto;
    
    // avancando a posicao do buffer
    //p++; 
    in = (in + 1) % N_BUFFER;

    // 
    // TODO: liberar o acesso ao buffer
    //
    
    Signal(mutex);
    //
    // TODO: liberar para o consumidor acessar o buffer
    //
    Signal(full);

}

int gera_rand(int limit)
{
    // 0 a (limit - 1)
    return rand() % limit;
}

void print_buffer()
{
    int i;
    printf("\t== BUFFER ==\n");
    for (i = 0; i < N_BUFFER; i++)
    {
        printf("\ti: %d | v: %d\n",i,buffer[i]);
    }
    printf("\n");
}
