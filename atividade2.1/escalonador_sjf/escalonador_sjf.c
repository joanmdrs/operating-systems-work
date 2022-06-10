#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "escalonador_sjf.h"

int n;

/**
 * Insere processo no final da fila passada
 */
BCP *anexa_fila (BCP *processo, BCP *fila){
  BCP *aux;
  processo -> prox = NULL;
  if (fila == NULL)
  {
          fila = processo;
          return fila;
  }
  for (aux = fila; aux->prox != NULL; aux = aux->prox);
  aux->prox = processo;
  return fila;
}

/**
 * Inicializa os campos de um processo
 *
 * Principais pontos a analisar:
 *
 * - Define a faixa dele (se � CPU ou I/O Bound); e
 * - Define o tempo que o processo precisar� para finalizar sua tarefa
 *   (tempo_restante).
 */
void cria_processo (BCP *processo){       
  static int id = 0; 
  processo->pid = id; /* pid do processo */
  processo->faixa = 2.0 * (double) random() / (double) RAND_MAX; /* faixa do processo */
  if (processo->faixa < 1)
    sprintf(processo->tipo, "I/O Bound");
  else
    sprintf(processo->tipo, "CPU Bound");
  processo->tempo_restante = (100 * (double) random() / (double) RAND_MAX);
  processo->tempo_espera = 0;
  processo->inicio.tv_sec = processo->inicio.tv_usec = 0;
  processo->fim.tv_sec = processo->fim.tv_usec = 0;
  processo->prox = NULL;  
    
  id++; /* Incrementa o PID para ser atribuido ao proximo processo */
  return;
}

/**
 * Mata um processo, apagando ele da memória
 */
void destroi_processo (BCP *processo){
  free (processo);
  return;
}

/**
 * Imprime na tela as estatásticas de um processo.
 *
 * É usado tanto quando um processo é finalizado, quanto no final de toda a
 * simulação, exibindo um sumário, onde é exibido o tempo médio de espera.
 */
void estatisticas (BCP *processo, int imprime_sumario){
  static int numero_processos = 0;
  static int num_cpu=0;
  static int num_io=0;
  static double tempo_total=0;
  static double tempo_cpu=0;
  static double tempo_io=0;
  if(!imprime_sumario){
    printf ("\nID do processo: %d\n", processo->pid);
    printf ("Tipo de processo: %s (faixa = %.3f)\n", processo->tipo, processo->faixa);
    printf ("Tempo total do processo:                    %f s\n", processo->tempo_restante);
    printf ("Tempo de espera:                    %f s\n", processo->tempo_espera);
    printf ("______________________________________________\n");
    numero_processos++;
    if (processo->faixa < 1){
      tempo_io += processo->tempo_espera;
      num_io++;
    }
    else{
      tempo_cpu += processo->tempo_espera;
      num_cpu++;
    }
    tempo_total += processo->tempo_espera;
    return;
  }
  else{
    printf ("\nSumario: \n\n");
    printf ("Total de processos: %d\n",numero_processos);
    printf ("Total de processos CPU Bound: %d\n",num_cpu);
    printf ("Total de processos I/O Bound: %d\n\n",num_io);
    printf ("Tempo medio de espera: %f s\n",(double)tempo_total/numero_processos);
    if(num_cpu)
      printf ("Tempo medio de espera de processos CPU Bound: %f s\n",(double)tempo_cpu/num_cpu);
    if(num_io)
      printf ("Tempo medio de espera de processos I/O Bound: %f s\n",(double)tempo_io/num_io);
    printf ("\n");
    return;
  }
}

/**
 * Calcula o tempo em que o processo estava aguardando para ser processado. 
 *
 * Efetua a subtra��o do tempo final pelo tempo inicial da contagem de espera.
 */
BCP *inicializa_tempo (BCP *processo){
  if((processo->inicio.tv_sec != 0) && (processo->inicio.tv_usec != 0)){
    gettimeofday (&(processo->fim), NULL);
    processo->tempo_espera += (double)(processo->fim.tv_sec + processo->fim.tv_usec*1.e-6) - (processo->inicio.tv_sec + processo->inicio.tv_usec*1.e-6);
  }
  return processo;
}

/**
 * Gera um tempo aleat�rio para simular o tempo em que um processo passar� na CPU.
 *
 * Essencial para simular os eventos que o processo poder� ter durante sua
 * execu��o, como para saber se ele continuar� em execu��o ou se necessitar�
 * bloquear por uma entrada e sa�da (E/S).
 *
 * Ponto a analisar:
 *  - Processos CPU Bound t�m uma maior probabilidade de continuar executando,
 *  pois gerar� um tempo de processador maior que QUANTUM;
 *  - Processos I/O Bound t�m uma maior probabilidade de gerar um tempo
 *  aleat�rio menor que o QUANTUM, o que o levaria a mais eventos de E/S.
 *
 * Esta probabilidade � dada pela utiliza��o da "faixa" de cada processo
 * (I/O ou CPU Bound) na gera��o do tempo.
 */
double gera_tempo (BCP *processo)
{
  double tempo_processador;
  tempo_processador = PI + (PI/2) + QUANTUM * processo->faixa * (double)random()/(double)RAND_MAX;
  
  /*
    * A inclusao das constantes no valor calculado acima pode ser
    * entendido como que constantes empiricas para otimizacao do
    * fator de uso de processador a cada vez que o processo e
    * escalonado.
    */
  
  return tempo_processador;
}

/**
 * Modifica��o 1: Inicia a contagem do tempo de espera mesmo quando o processo
 * entra na fila1.
 *
 * Essencial para os algoritmos que possuem apenas uma fila (FIFO, SJF, RR).
 */
BCP * conta_espera(BCP *fila1)
{
  BCP * p = NULL;
  for (p = fila1; p != NULL; p = p->prox)
    gettimeofday (&(p->inicio), NULL);

  return fila1;
}

/**
 * Fun��o que efetua o algoritmo de escalonamento dos processos.
 *
 * Principal ponto de modifica��o para a cria��o dos demais algoritmos
 * solicitados.
 */
void escalona (BCP *fila1){
  /**
    * Ponteiro que vai apontar para o processo que estar� em execu��o.
    *
    * � importante observar que este ponteiro simula o processo que est�
    * no estado "executando", onde s� pode haver um processo por vez.
    */
  int tempo, inicio, fim, shortest, i;
  double tempo_processador;
  BCP *copia, *tmpsrc, *tmp, *beforeshortest;

  tmpsrc = fila1;
  copia = tmp = NULL;

  while (tmpsrc != NULL) {
    if (copia == NULL) {
        copia = tmpsrc;
        tmp = copia;
    }  else {
        tmp->prox = tmpsrc;
        tmp = tmp->prox;
    };
    tmpsrc = tmpsrc->prox;    
  }

  while (copia != NULL) {
    beforeshortest = NULL;
    shortest = copia->tempo_restante;
    tmp = copia->prox;
    tmpsrc = copia;
    while (tmp != NULL) {
      if (tmp->tempo_restante < shortest) {
        shortest = tmp->tempo_restante;
        beforeshortest = tmpsrc;
      };
      tmpsrc = tmp;
      tmp = tmp->prox;
    };

    if (beforeshortest == NULL) {
      copia=inicializa_tempo(copia);

      tempo_processador = gera_tempo(copia);

      for (i=0; i < (int) tempo_processador; i++);
      estatisticas(copia, SEMSUMARIO);
      tmpsrc = copia;
      copia = copia->prox;
      destroi_processo(tmpsrc);        
    } else {
    /* Aloca o primeiro processo caso não haja 
    ocorrencia de outro menor 
    */
    tmp = beforeshortest->prox;
    tmp=inicializa_tempo(tmp);

    tempo_processador = gera_tempo(tmp);

    for (i=0; i < (int) tempo_processador; i++);
    estatisticas(tmp, SEMSUMARIO);
    beforeshortest->prox = tmp->prox;
    destroi_processo(tmp);
    }
  }
  return;
}

 /*
 * Fun��o principal do programa.
 *
 * Realiza a chamada para a execu��o das demais fun��es.
 */
int main(int argc, char **argv){
  // Vari�vel de controle, para o for
  int i;
  // Estruturas que armazenar�o o inicio e fim da simula��o
  struct timeval comeco, fim;

  // Estrutura para manipular um processo
  BCP *processo;

  // Ponteiro para o in�nio da fila 1
  BCP *fila1 = NULL;

          
  // S� permite o programa executar se seu uso for correto
  if (argc != 2){
    printf("\nUso: ./escalonador [numero de processos]\n");
    printf("Uso recomendado: ./escalonador [numero de processos] | less\n\n");
    exit (1);
  }
  
  // Obtem o n�mero de processos informados na linha de comando
  n = atoi(argv[1]);      /* Numero de processos que serao executados */

  if(n<=0){
    printf("\nPor favor entre com um valor coerente...\n\n");
    exit(1);
  }
  
  srand (time(NULL));     /* Gera uma nova semente de numeros aleatorios */
  
  /**
    * La�o para criar os n processos solicitados:
    */
  for (i = 0; i < n; i++){
    // 1 - Cria o processo, alocando mem�ria para sua estrutura
    processo = (BCP *) malloc (sizeof (BCP));
    // 2 - Preenche os dados iniciais do processo
    cria_processo (processo);
    // 3 - Insere o processo no final da fila1
    fila1 = anexa_fila (processo, fila1);
    // 4 - Incrementa a quantidade de vezes que o processo entrou na fila1, neste caso 1 vez.
  }
  
  /* Contagem do tempo de simulacao */
  gettimeofday(&comeco, NULL);
  
  /* Inicializacao da simulacao */;
  escalona (fila1);

  /* Fim da contagem */
  gettimeofday(&fim, NULL);

  // Imprime na tela um sum�rio de toda a simula��o realizada
  estatisticas(NULL, COMSUMARIO);

  printf("Tempo total de simulacao: %f s\n\n",(double)(fim.tv_sec + fim.tv_usec*1.e-6) -
  (comeco.tv_sec + comeco.tv_usec*1.e-6));
          
  return 0;
}
