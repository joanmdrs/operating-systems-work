#define QUANTUM 10
#define SEMSUMARIO 0
#define COMSUMARIO !SEMSUMARIO
#define PI 3.14159265359

typedef struct bcp{
        unsigned int pid;               
        char tipo[10];                  
        double faixa;                   
        unsigned int num_fila1;         
        unsigned int num_fila2;         
        unsigned int num_bloqueado;     
        double tempo_restante;          
        double tempo_espera;            
        struct timeval inicio;          
        struct timeval fim;             
        struct bcp *prox;               
        struct bcp *ante;               
} BCP;

BCP *anexa_fila (BCP *processo, BCP *fila);

void cria_processo (BCP *processo);

void destroi_processo (BCP *processo);

void estatisticas (BCP *processo, int imprime_sumario);

int interrupcao (BCP *bloqueados);

BCP *inicializa_tempo (BCP *processo);

double gera_tempo (BCP *processo);

void escalona (BCP *fila1, BCP *fila2, BCP *bloqueados);
