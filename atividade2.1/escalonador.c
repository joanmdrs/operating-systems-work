#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "escalonador.h"

int n;

/**
 * Insere processo no final da fila passada
 */
BCP *anexa_fila (BCP *processo, BCP *fila)
{
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
 * - Define a faixa dele (se é CPU ou I/O Bound); e
 * - Define o tempo que o processo precisará para finalizar sua tarefa
 *   (tempo_restante).
 */
void cria_processo (BCP *processo)
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
        processo->prox = NULL;  
         
        id++; /* Incrementa o PID para ser atribuido ao proximo processo */
        return;
}

/**
 * Mata um processo, apagando ele da memória
 */
void destroi_processo (BCP *processo)
{
        free (processo);
        return;
}

/**
 * Imprime na tela as estatísticas de um processo.
 *
 * É usado tanto quando um processo é finalizado, quanto no final de toda a
 * simulação, exibindo um sumário, onde é exibido o tempo médio de espera.
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
 * Simula a ocorrência de uma interrupção para um processo que está bloqueado.
 *
 * Essencial para que o processo saia do estado bloqueado e volte à fila de
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
 * Efetua a subtração do tempo final pelo tempo inicial da contagem de espera.
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
 * Gera um tempo aleatório para simular o tempo em que um processo passará na CPU.
 *
 * Essencial para simular os eventos que o processo poderá ter durante sua
 * execução, como para saber se ele continuará em execução ou se necessitará
 * bloquear por uma entrada e saída (E/S).
 *
 * Ponto a analisar:
 *  - Processos CPU Bound têm uma maior probabilidade de continuar executando,
 *  pois gerará um tempo de processador maior que QUANTUM;
 *  - Processos I/O Bound têm uma maior probabilidade de gerar um tempo
 *  aleatório menor que o QUANTUM, o que o levaria a mais eventos de E/S.
 *
 * Esta probabilidade é dada pela utilização da "faixa" de cada processo
 * (I/O ou CPU Bound) na geração do tempo.
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
 * Modificação 1: Inicia a contagem do tempo de espera mesmo quando o processo
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
 * Função que efetua o algoritmo de escalonamento dos processos.
 *
 * Principal ponto de modificação para a criação dos demais algoritmos
 * solicitados.
 */
void escalona (BCP *fila1, BCP *fila2, BCP *bloqueados)
{
        /**
         * Ponteiro que vai apontar para o processo que estará em execução.
         *
         * É importante observar que este ponteiro simula o processo que está
         * no estado "executando", onde só pode haver um processo por vez.
         */
        BCP *processo = NULL;

        // Variáveis de controle para os laços
        int executa = 1, i;

        // Armazenará o valor gerado para simular o tempo que o processo
        // passará processando.
        double tempo_processador;

        // Modificação 2: Inicia a contagem do tempo de espera já quando os
        // processos estão na fila1
        fila1 = conta_espera(fila1);

        // Executará o programa enquanto ainda houver processos necessitando
        // ser processador, quando seu tempo_restante for maior que 0
        while (executa)
        {
                // Processa os processos da fila1.
                //
                // Pontos a analisar:
                //
                // 1 - Executará enquanto houver processos nesta fila;
                // 2 - Sempre executará os processos da fila1 antes da fila2, dando prioridade à fila1.
                while (fila1 != NULL)
                {
                        // Busca se há algum processo na fila de bloqueados,
                        // e se houve a interrupção necessária para tornar o
                        // processo apto novamente.
                        //
                        // Ponto a analisar:
                        // 1 - Este teste é feito a cada vez que o laço
                        // inicia, mas o processo que estava bloqueado é
                        // inserido no final da fila de aptos (fila1);
                        // 2 - A interrupção é simulada por meio da geração de
                        // um valor aleatório.
                        if ((bloqueados != NULL) && (interrupcao(bloqueados)))
                        {
                                // Pega o primeiro processo da fila de
                                // bloqueados
                                processo = bloqueados;
                                // Anda a fila de bloqueados
                                bloqueados = processo->prox;
                                // Inicia a contagem do tempo de espera em que
                                // o processo entre na fila, até que seja processado.
                                gettimeofday(&(processo -> inicio), NULL);
                                // Incrementa a quantidade de vezes que o
                                // processo entrou na fila1
                                processo->num_fila1++;
                                // Insere o processo no final da fila1
                                fila1 = anexa_fila (processo, fila1);
                        }

                        /** Inicio da simulação de execução do processo **/
                        
                        // Pega primeiro processo da fila1, simula colocar o
                        // processo no estado executando
                        processo = fila1;
                        
                        // Retira o processo da fila1, anda a fila
                        fila1 = processo -> prox;
                        
                        // Contabiliza o tempo esperado pelo processo, desde
                        // que entrou na fila até agora, quando inicia seu
                        // estado de execução.
                        processo=inicializa_tempo(processo);
                        
                        // Simula quanto tempo o processo irá passar
                        // executando no processador.
                        //
                        // Este valor definirá o que ocorrerá com o processo.
                        tempo_processador = gera_tempo(processo);

                        // Verificação se o tempo de processador gerado é
                        // maior do que o QUANTUM.
                        //
                        // O processo necessitará de um tempo de processamento
                        // maior do que o permitido, que é o valor do QUANTUM
                        // (10 segundos).
                        // 
                        // Ponto a analisar:
                        // 1 - Neste caso, o processo usou todo o seu QUANTUM, 
                        // logo vai para o fim da fila2, perdendo prioridade
                        // para os processos que estão na fila1
                        // 2 - Não é permitido que o processo passe mais tempo
                        // executando do que o valor do QUANTUM, logo o
                        // tempo_processador é substituído pelo valor do
                        // QUANTUM.
                        if (tempo_processador > QUANTUM)
                        {
                                // Reduz do tempo restante do processador
                                // apenas o valor máximo permitido para
                                // processamento, que é o QUANTUM.
                                //
                                // Simula que o processador utilizou todo o
                                // seu QUANTUM de tempo máximo permitido.
                                processo->tempo_restante -= QUANTUM;

                                // Simulação da execução do processo, um "for"
                                // sem nenhuma ação.
                                //
                                // Modificação 3: Executa apenas o tempo
                                // máximo do QUANTUM, não mais o tempo
                                // excedente gerado pelo tempo_processador.
                                for (i=0; i < (int) QUANTUM; i++);
                                
                                // Modificação 4: Verifica se a execução do
                                // processo foi suficiente para que sua tarefa
                                // seja concluída.
                                //
                                // Caso não falte mais nada para fazer, 
                                // o tempo_restante do processo será zerado,
                                // ou negativo.
                                if (processo->tempo_restante <= 0)
                                {
                                    // Neste caso, a execucao está terminada,
                                    // Sendo o processo finalizado e suas
                                    // estatísticas individuais exibidas na
                                    // tela.
                                    estatisticas(processo, SEMSUMARIO);
                                    destroi_processo(processo);
                                }
                                else // AVISO: este else estava faltando no codigo
                                {
                                    // Ponto a analisa:
                                    // 1 - Neste ponto simula-se a saída do processo
                                    // do estado de executando para apto, mas
                                    // neste caso, ele perde prioridade quando usa
                                    // todo o seu QUANTUM, indo para o final da
                                    // fila2.
                                    
                                    // Incrementa a contagem de vezes que o
                                    // processo entrou na fila2
                                    processo->num_fila2++;
                                    
                                    // Inicia a contagem do tempo de espera do
                                    // processo em fila
                                    gettimeofday(&(processo -> inicio), NULL);

                                    // Insere o processo no final da fila2
                                    fila2=anexa_fila(processo, fila2);
                                }
                        }
                        else
                        {       
                                // Diminui do tempo restante de processamento
                                // o tempo de processador que o processo
                                // conseguiu.
                                processo->tempo_restante -= tempo_processador;
                                
                                // Roda a simulação da execução do processo.
                                //
                                // Modificação 5: roda a simulação mesmo que
                                // ele vá terminar depois disso.
                                for (i=0; i < (int) tempo_processador; i++);

                                // Caso o processo não tenha utilizado todo o
                                // QUANTUM disponível para processamento, é
                                // simulado que duas possíveis situações
                                // ocorreu:
                                
                                // 1 - Ele finalizou, quando não há mais tempo
                                // restante de processamento a fazer.
                                if (processo->tempo_restante <= 0)
                                {       
                                    // Neste caso, finaliza o processo
                                    estatisticas(processo, SEMSUMARIO);
                                    destroi_processo(processo);
                                }
                                // 2 - Ele parou para realizar uma E/S
                                else
                                {
                                    // Adiciona o processo na fila de
                                    // bloqueados, até que ocorra uma
                                    // interrupção que o retorne para a fila
                                    // de aptos.
                                    //
                                    // Ponto a analisar:
                                    // 1 - A fila de processos bloqueados
                                    // segue uma sequencia FIFO, já que o
                                    // primeiro processo a entrar nesta fila
                                    // será sempre o primeiro a ser servido,
                                    // retornando para a fila de aptos.
                                    processo->num_bloqueado++;
                                    bloqueados=anexa_fila(processo, bloqueados);
                                }
                        }
                }

                // Processamento dos processos que estão na fila2
                //
                // Ponto a analisar:
                // 1 - Este processos possuem menor prioridade do que os
                // processos da fila2.
                // 2 - Só serão processados os processos da fila2 se não
                // houver processos na fila1.
                //         
                // Os procedimentos realizados na fila2 são os mesmos dos
                // comentados acima para a fila1, só que focando na fila2.
                while (fila2 != NULL && fila1 == NULL)
                {
                        // Verifica se há interrupções para processos
                        // bloqueados, e os retorna para a fila1.
                        //
                        // Ponto a analisar:
                        // 1 - Os processos bloqueados sempre retornam para a
                        // fila1.
                        if ((bloqueados != NULL) && (interrupcao(bloqueados)))
                        {
                                processo = bloqueados;
                                bloqueados = processo->prox;
                                gettimeofday(&(processo -> inicio), NULL);
                                processo->num_fila1++;
                                fila1 = anexa_fila (processo, fila1);
                        }

                        // Coloca o primeiro processo da fila2 em execução.
                        processo = fila2;

                        // Anda a fila2.
                        fila2 = processo -> prox;

                        // Contabiliza tempo de espera.
                        processo=inicializa_tempo(processo);

                        // Gera tempo simulado de processador
                        tempo_processador = gera_tempo(processo);

                        // Verifica se o processo utilizará mais tempo do que
                        // o permitido em QUANTUM
                        if (tempo_processador > QUANTUM)
                        {       
                                // Reduz o tempo de QUANTUM do tempo restante de processamento, 
                                // o processo utilizou todo o seu QUANTUM.
                                processo->tempo_restante -= QUANTUM;

                                // Modificação 6: Somente executa o tempo do
                                // QUANTUM, não mais o excesso gerado.
                                for (i=0; i < (int) QUANTUM; i++);
                                
                                // Modificação 7: Verifica se o processo
                                // finalizou o seu tempo restante, devendo ser
                                // finalizado.
                                if (processo->tempo_restante <= 0)
                                {       
                                        /* Execucao terminada */
                                        estatisticas(processo, SEMSUMARIO);
                                        destroi_processo(processo);
                                }
                                else
                                {
                                    // Insere o processo no final da fila2.
                                    //
                                    // Ponto a analisar:
                                    // 1 - Se o processo já está na fila2, ele
                                    // retornará para o final desta mesma fila.
                                    processo->num_fila2++;
                                    gettimeofday(&(processo -> inicio), NULL);
                                    fila2 = anexa_fila(processo, fila2);
                                }
                        }
                        // Caso o processo não tenha utilizado todo o seu
                        // tempo, ou ele terminou ou irá esperar por E/S.
                        else
                        {       
                                // Reduz do tempo restante o tempo simulador
                                // de processamento.
                                processo->tempo_restante -= tempo_processador;
                                
                                // Modificação 8: Simula o processamento
                                // independente se o processo foi termimar
                                // após isto ou se ele for para bloqueado.
                                for (i=0; i < (int) tempo_processador; i++);

                                // Verifica se o processo irá ser finalizado.
                                if (processo->tempo_restante <= 0)
                                {       
                                        /* Execucao terminada */
                                        estatisticas(processo, SEMSUMARIO);
                                        destroi_processo(processo);
                                }
                                // Ou se o processo irá entrar em estado
                                // bloqueado, aguardando por uma E/S.
                                else
                                {
                                        processo->num_bloqueado++;
                                        bloqueados=anexa_fila(processo, bloqueados);
                                }
                        }
                }

                // Verifica as condições para finalização do escalonador
                //
                // Se não houver nenhum processo nas filas 1 e 2
                if ((fila1 == NULL) && (fila2 == NULL))
                {
                        // E se também não houver nenhum processo na fila de
                        // bloqueados,
                        if (bloqueados == NULL)
                                // Todos os processos foram finalizados,
                                // devendo o escalonador ser finalizado
                                // também, setando esta variável para
                                // finalizar a execução do laço maior.
                                executa = 0;
                        // Caso ainda haja processos bloqueados, retirá-los
                        // para a fila de aptos (fila1).
                        else
                        {
                                processo = bloqueados;
                                bloqueados = processo->prox;
                                gettimeofday(&(processo -> inicio), NULL);
                                processo->num_fila1++;
                                fila1 = anexa_fila (processo, fila1);   
                        }
                }
        }

        return;
}

/**
 * Função principal do programa.
 *
 * Realiza a chamada para a execução das demais funções.
 */
int main(int argc, char **argv)
{
        // Variável de controle, para o for
        int i;
        // Estruturas que armazenarão o inicio e fim da simulação
        struct timeval comeco, fim;

        // Estrutura para manipular um processo
        BCP *processo;

        // Ponteiro para o inínio da fila 1
        BCP *fila1 = NULL;
        
        // Ponteiro para o inínio da fila 2
        BCP *fila2 = NULL;
        
        // Ponteiro para o inínio da fila de bloqueados
        BCP *bloqueados = NULL;
                
        // Só permite o programa executar se seu uso for correto
        if (argc != 2)
        {
                printf("\nUso: ./escalonador [numero de processos]\n");
                printf("Uso recomendado: ./escalonador [numero de processos] | less\n\n");
                exit (1);
        }
        
        // Obtem o número de processos informados na linha de comando
        n = atoi(argv[1]);      /* Numero de processos que serao executados */

        if(n<=0)
        {
                printf("\nPor favor entre com um valor coerente...\n\n");
                exit(1);
        }
        
        srand (time(NULL));     /* Gera uma nova semente de numeros aleatorios */
        
        /**
         * Laço para criar os n processos solicitados:
         */
        for (i = 0; i < n; i++)
        {
                // 1 - Cria o processo, alocando memória para sua estrutura
                processo = (BCP *) malloc (sizeof (BCP));
                // 2 - Preenche os dados iniciais do processo
                cria_processo (processo);
                // 3 - Insere o processo no final da fila1
                fila1 = anexa_fila (processo, fila1);
                // 4 - Incrementa a quantidade de vezes que o processo entrou na fila1, neste caso 1 vez.
                processo->num_fila1++;
        }
        
        /* Contagem do tempo de simulacao */
        gettimeofday(&comeco, NULL);
        
        /* Inicializacao da simulacao */;
        escalona (fila1, fila2, bloqueados);

        /* Fim da contagem */
        gettimeofday(&fim, NULL);

        // Imprime na tela um sumário de toda a simulação realizada
        estatisticas(NULL, COMSUMARIO);

        printf("Tempo total de simulacao: %f s\n\n",(double)(fim.tv_sec + fim.tv_usec*1.e-6) -
        (comeco.tv_sec + comeco.tv_usec*1.e-6));
                
        return 0;
}
