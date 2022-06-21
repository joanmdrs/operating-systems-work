#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "escalonador_prio.h"

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
void cria_processo (BCP *processo, int priori)
{       
  static int id = 0; 
  processo->pid = id; /* pid do processo */
  processo->faixa = 2.0 * (double) random() / (double) RAND_MAX; /* faixa do processo */
  if (processo->faixa < 1)
    sprintf(processo->tipo, "I/O Bound");
  else
    sprintf(processo->tipo, "CPU Bound");
  processo->num_fila1 = 0; 
  processo->num_fila2 = 0;
  processo->num_bloqueado = 0;
  processo->tempo_restante = (100 * (double) random() / (double) RAND_MAX);
  processo->tempo_espera = 0;
  processo->inicio.tv_sec = processo->inicio.tv_usec = 0;
  processo->fim.tv_sec = processo->fim.tv_usec = 0;
  processo->prioridade = priori;
  processo->prox = NULL;  
    
  id++; /* Incrementa o PID para ser atribuido ao proximo processo */
  return;
}

/**
 * Mata um processo, apagando ele da mem�ria
 */
void destroi_processo (BCP *processo)
{
  free (processo);
  return;
}

/**
 * Imprime na tela as estat�sticas de um processo.
 *
 * � usado tanto quando um processo � finalizado, quanto no final de toda a
 * simula��o, exibindo um sum�rio, onde � exibido o tempo m�dio de espera.
 */
void estatisticas (BCP *processo, int imprime_sumario)
{
  static int numero_processos = 0;
  static int num_cpu=0;
  static int num_io=0;
  static double tempo_total=0;
  static double tempo_cpu=0;
  static double tempo_io=0;
  if(!imprime_sumario)
  {
    printf ("\nID do processo: %d\n", processo->pid);
    printf ("Tipo de processo: %s (faixa = %.3f)\n", processo->tipo, processo->faixa);
    printf ("Prioridade do processo: %d\n", processo->prioridade);
    printf ("Passagens pela fila 1:                  %d\n", processo->num_fila1);
    printf ("Passagens pela fila 2:                  %d\n", processo->num_fila2);
    printf ("Numero de vezes que foi bloqueado:      %d\n", processo->num_bloqueado);
    printf ("Tempo de espera:                    %f s\n", processo->tempo_espera);
    printf ("______________________________________________\n");
    numero_processos++;
    if (processo->faixa < 1)
    {
      tempo_io += processo->tempo_espera;
      num_io++;
    }
    else
    {
      tempo_cpu += processo->tempo_espera;
      num_cpu++;
    }
    tempo_total += processo->tempo_espera;
    return;
  }
  else
  {
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
 * Simula a ocorr�ncia de uma interrup��o para um processo que est� bloqueado.
 *
 * Essencial para que o processo saia do estado bloqueado e volte � fila de
 * aptos.
 */
int interrupcao (BCP *bloqueados)
{
  int randomico;
  if (bloqueados == NULL)
    return 0;
  randomico = (int) (9.0 * (double) random() / (double) RAND_MAX);
  if (randomico >= 5)
    return 1;
  else
    return 0;
}

/**
 * Calcula o tempo em que o processo estava aguardando para ser processado. 
 *
 * Efetua a subtra��o do tempo final pelo tempo inicial da contagem de espera.
 */
BCP *inicializa_tempo (BCP *processo)
{
  if((processo->inicio.tv_sec != 0) && (processo->inicio.tv_usec != 0))
  {
    gettimeofday (&(processo->fim), NULL);
    processo->tempo_espera += (double)(processo->fim.tv_sec + processo->fim.tv_usec*1.e-6) -
    (processo->inicio.tv_sec + processo->inicio.tv_usec*1.e-6);
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
void escalona (BCP *fila1, BCP *fila2, BCP *bloqueados){
  /**
    * Ponteiro que vai apontar para o processo que estar� em execu��o.
    *
    * � importante observar que este ponteiro simula o processo que est�
    * no estado "executando", onde s� pode haver um processo por vez.
    */
  BCP *processo = NULL;

  // Vari�veis de controle para os la�os
  int executa = 1, i;

  // Armazenar� o valor gerado para simular o tempo que o processo
  // passar� processando.
  double tempo_processador;

  // Modifica��o 2: Inicia a contagem do tempo de espera j� quando os
  // processos est�o na fila1
  fila1 = conta_espera(fila1);
  fila2 = conta_espera(fila2);

  // Executar� o programa enquanto ainda houver processos necessitando
  // ser processador, quando seu tempo_restante for maior que 0
  while (executa){
    // Processa os processos da fila1.
    //
    // Pontos a analisar:
    //
    // 1 - Executar� enquanto houver processos nesta fila;
    // 2 - Sempre executar� os processos da fila1 antes da fila2, dando prioridade � fila1.
    while (fila1 != NULL){
      // Busca se h� algum processo na fila de bloqueados,
      // e se houve a interrup��o necess�ria para tornar o
      // processo apto novamente.
      //
      // Ponto a analisar:
      // 1 - Este teste � feito a cada vez que o la�o
      // inicia, mas o processo que estava bloqueado �
      // inserido no final da fila de aptos (fila1);
      // 2 - A interrup��o � simulada por meio da gera��o de
      // um valor aleat�rio.
      if ((bloqueados != NULL) && (interrupcao(bloqueados))){
        processo = bloqueados;
        bloqueados = processo->prox;
        if(processo->prioridade <= 8){
          gettimeofday(&(processo -> inicio), NULL);
          processo->num_fila1++;
          fila1 = anexa_fila (processo, fila1);
        } else {
          gettimeofday(&(processo -> inicio), NULL);
          processo->num_fila2++;
          fila2 = anexa_fila (processo, fila2);
        }
      }

      /** Inicio da simula��o de execu��o do processo **/
      
      // Pega primeiro processo da fila1, simula colocar o
      // processo no estado executando
      processo = fila1;
      
      // Retira o processo da fila1, anda a fila
      fila1 = processo -> prox;
      
      // Contabiliza o tempo esperado pelo processo, desde
      // que entrou na fila at� agora, quando inicia seu
      // estado de execu��o.
      processo=inicializa_tempo(processo);
      
      // Simula quanto tempo o processo ir� passar
      // executando no processador.
      //
      // Este valor definir� o que ocorrer� com o processo.
      tempo_processador = gera_tempo(processo);

      // Verifica��o se o tempo de processador gerado �
      // maior do que o QUANTUM.
      //
      // O processo necessitar� de um tempo de processamento
      // maior do que o permitido, que � o valor do QUANTUM
      // (10 segundos).
      // 
      // Ponto a analisar:
      // 1 - Neste caso, o processo usou todo o seu QUANTUM, 
      // logo vai para o fim da fila2, perdendo prioridade
      // para os processos que est�o na fila1
      // 2 - N�o � permitido que o processo passe mais tempo
      // executando do que o valor do QUANTUM, logo o
      // tempo_processador � substitu�do pelo valor do
      // QUANTUM.
      if (tempo_processador > QUANTUM){
        // Reduz do tempo restante do processador
        // apenas o valor m�ximo permitido para
        // processamento, que � o QUANTUM.
        //
        // Simula que o processador utilizou todo o
        // seu QUANTUM de tempo m�ximo permitido.
        processo->tempo_restante -= QUANTUM;

        // Simula��o da execu��o do processo, um "for"
        // sem nenhuma a��o.
        //
        // Modifica��o 3: Executa apenas o tempo
        // m�ximo do QUANTUM, n�o mais o tempo
        // excedente gerado pelo tempo_processador.
        for (i=0; i < (int) QUANTUM; i++);
        
        // Modifica��o 4: Verifica se a execu��o do
        // processo foi suficiente para que sua tarefa
        // seja conclu�da.
        //
        // Caso n�o falte mais nada para fazer, 
        // o tempo_restante do processo ser� zerado,
        // ou negativo.
        if (processo->tempo_restante <= 0){
            // Neste caso, a execucao est� terminada,
            // Sendo o processo finalizado e suas
            // estat�sticas individuais exibidas na
            // tela.
            estatisticas(processo, SEMSUMARIO);
            destroi_processo(processo);
        } else {
            // Ponto a analisa:
            // 1 - Neste ponto simula-se a sa�da do processo
            // do estado de executando para apto, mas
            // neste caso, ele perde prioridade quando usa
            // todo o seu QUANTUM, indo para o final da
            // fila2.
            
            // Inicia a contagem do tempo de espera do
            // processo em fila
            gettimeofday(&(processo -> inicio), NULL);

            // Insere o processo no final da fila2
            fila1=anexa_fila(processo, fila1);
        }
      } else{       
        // Diminui do tempo restante de processamento
        // o tempo de processador que o processo
        // conseguiu.
        processo->tempo_restante -= tempo_processador;
        
        // Roda a simula��o da execu��o do processo.
        //
        // Modifica��o 5: roda a simula��o mesmo que
        // ele v� terminar depois disso.
        for (i=0; i < (int) tempo_processador; i++);

        // Caso o processo n�o tenha utilizado todo o
        // QUANTUM dispon�vel para processamento, �
        // simulado que duas poss�veis situa��es
        // ocorreu:
        
        // 1 - Ele finalizou, quando n�o h� mais tempo
        // restante de processamento a fazer.
        if (processo->tempo_restante <= 0){       
          // Neste caso, finaliza o processo
          estatisticas(processo, SEMSUMARIO);
          destroi_processo(processo);
        } else {
          // Adiciona o processo na fila de
          // bloqueados, at� que ocorra uma
          // interrup��o que o retorne para a fila
          // de aptos.
          //
          // Ponto a analisar:
          // 1 - A fila de processos bloqueados
          // segue uma sequencia FIFO, j� que o
          // primeiro processo a entrar nesta fila
          // ser� sempre o primeiro a ser servido,
          // retornando para a fila de aptos.
          processo->num_bloqueado++;
          bloqueados=anexa_fila(processo, bloqueados);
        }
      }
    }

    // Processamento dos processos que est�o na fila2
    //
    // Ponto a analisar:
    // 1 - Este processos possuem menor prioridade do que os
    // processos da fila2.
    // 2 - S� ser�o processados os processos da fila2 se n�o
    // houver processos na fila1.
    //         
    // Os procedimentos realizados na fila2 s�o os mesmos dos
    // comentados acima para a fila1, s� que focando na fila2.
    while (fila2 != NULL && fila1 == NULL){
      // Verifica se h� interrup��es para processos
      // bloqueados, e os retorna para a fila1.
      //
      // Ponto a analisar:
      // 1 - Os processos bloqueados sempre retornam para a
      // fila1.
      if ((bloqueados != NULL) && (interrupcao(bloqueados))){
        processo = bloqueados;
        bloqueados = processo->prox;
        if(processo->prioridade <= 8){
          gettimeofday(&(processo -> inicio), NULL);
          processo->num_fila1++;
          fila1 = anexa_fila (processo, fila1);
        } else {
          gettimeofday(&(processo -> inicio), NULL);
          processo->num_fila2++;
          fila2 = anexa_fila (processo, fila2);
        }
      }

      // Coloca o primeiro processo da fila2 em execu��o.
      processo = fila2;

      // Anda a fila2.
      fila2 = processo -> prox;

      // Contabiliza tempo de espera.
      processo=inicializa_tempo(processo);

      // Gera tempo simulado de processador
      tempo_processador = gera_tempo(processo);

      // Caso o processo n�o tenha utilizado todo o seu
      // tempo, ou ele terminou ou ir� esperar por E/S.
   
      // Reduz do tempo restante o tempo simulador
      // de processamento.
      processo->tempo_restante -= tempo_processador;
      
      // Modifica��o 8: Simula o processamento
      // independente se o processo foi termimar
      // ap�s isto ou se ele for para bloqueado.
      for (i=0; i < (int) tempo_processador; i++);

      // Verifica se o processo ir� ser finalizado.
      if (processo->tempo_restante <= 0){       
        /* Execucao terminada */
        estatisticas(processo, SEMSUMARIO);
        destroi_processo(processo);
      }
      // Ou se o processo ir� entrar em estado
      // bloqueado, aguardando por uma E/S.
      else{
        processo->num_bloqueado++;
        bloqueados=anexa_fila(processo, bloqueados);
      }
    }

    // Verifica as condi��es para finaliza��o do escalonador
    //
    // Se n�o houver nenhum processo nas filas 1 e 2
    if ((fila1 == NULL) && (fila2 == NULL))
    {
      // E se tamb�m n�o houver nenhum processo na fila de
      // bloqueados,
      if (bloqueados == NULL)
        // Todos os processos foram finalizados,
        // devendo o escalonador ser finalizado
        // tamb�m, setando esta vari�vel para
        // finalizar a execu��o do la�o maior.
        executa = 0;
      // Caso ainda haja processos bloqueados, retir�-los
      // para a fila de aptos (fila1).
      else{
        processo = bloqueados;
        bloqueados = processo->prox;
        if(processo->prioridade > 8){
          gettimeofday(&(processo -> inicio), NULL);
          processo->num_fila2++;
          fila2 = anexa_fila (processo, fila2);
        } else{
          gettimeofday(&(processo -> inicio), NULL);
          processo->num_fila1++;
          fila1 = anexa_fila (processo, fila1);
        }
      }
    }
  }

  return;
}

/**
 * Fun��o principal do programa.
 *
 * Realiza a chamada para a execu��o das demais fun��es.
 */
int main(int argc, char **argv){

  int randomico;
  // Vari�vel de controle, para o for
  int i;
  // Estruturas que armazenar�o o inicio e fim da simula��o
  struct timeval comeco, fim;

  // Estrutura para manipular um processo
  BCP *processo;

  // Ponteiro para o in�nio da fila 1
  BCP *fila1 = NULL;
  
  // Ponteiro para o in�nio da fila 2
  BCP *fila2 = NULL;
  
  // Ponteiro para o in�nio da fila de bloqueados
  BCP *bloqueados = NULL;
          
  // S� permite o programa executar se seu uso for correto
  if (argc != 2)
  {
    printf("\nUso: ./escalonador [numero de processos]\n");
    printf("Uso recomendado: ./escalonador [numero de processos] | less\n\n");
    exit (1);
  }
  
  // Obtem o n�mero de processos informados na linha de comando
  n = atoi(argv[1]);      /* Numero de processos que serao executados */

  if(n<=0)
  {
    printf("\nPor favor entre com um valor coerente...\n\n");
    exit(1);
  }
  
  srand (time(NULL));     /* Gera uma nova semente de numeros aleatorios */
  
  /**
    * La�o para criar os n processos solicitados:
    */
  for (i = 0; i < n; i++){

    //Gera um numero aleatório entre 0 e 10
    randomico = (int) (10 * (double) random() / (double) RAND_MAX);
    // 1 - Cria o processo, alocando mem�ria para sua estrutura
    processo = (BCP *) malloc (sizeof (BCP));
    // 2 - Preenche os dados iniciais do processo passando também como parâmetro a prioridade gerada anteriormente
    cria_processo (processo, randomico);
    // 3 - Se o processo tiver uma prioridade menor ou igual a 8 ele irá para fila1 (80%) se não irá para fila2 (20%).
    if (randomico <= 8){
      fila1 = anexa_fila (processo, fila1);
      // 4 - Incrementa a quantidade de vezes que o processo entrou na fila1, neste caso 1 vez.
      processo->num_fila1++;
    } else {
      fila2 = anexa_fila (processo, fila2);
      // 4 - Incrementa a quantidade de vezes que o processo entrou na fila2, neste caso 1 vez.
      processo->num_fila2++;
    }
  }
  
  /* Contagem do tempo de simulacao */
  gettimeofday(&comeco, NULL);
  
  /* Inicializacao da simulacao */;
  escalona (fila1, fila2, bloqueados);

  /* Fim da contagem */
  gettimeofday(&fim, NULL);

  // Imprime na tela um sum�rio de toda a simula��o realizada
  estatisticas(NULL, COMSUMARIO);

  printf("Tempo total de simulacao: %f s\n\n",(double)(fim.tv_sec + fim.tv_usec*1.e-6) -
  (comeco.tv_sec + comeco.tv_usec*1.e-6));
          
  return 0;
}
