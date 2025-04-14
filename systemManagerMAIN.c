// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "functions.h"  


int main(int argc, char *argv[]) {
    //verifica se o config file foi passado como arg
       if (argc < 2) {
        puts("Usage: 5g_auth_platform <config-file>\n");
        exit(0);
    }

    log_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    //log file
    logFile = fopen("logfile.txt", "w");
    if (logFile == NULL) {
        erro("Erro a arbrir o log file");
    }
   
    output("System a comecar...");

    //config file
    output("A verificar o Config File");
    readConfig(argv[1]);

    //cria shm
     output("A Criar a Shared Memory");
    createSharedMemory();
    
    output("A Criar o semaforo da Shared Memory");
    createSHMsemaph();
    
    // criar message queue
    output("A Criar a Message Queue");
    createMsgQ();

    puts("A Criar as queues\n");
    // criar queues
    init_queue(&VideoStreamingQueue);
    init_queue(&OtherServicesQueue);

    puts("A Criar os mutexes");
    
    // iniciar mutexes e conditional variables
    queues_mutex                = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    authorization_engines_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    cond_queues                 = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
 

    puts("A Capturar signal\n");
    mainPID = getpid();
    signal(SIGINT, terHandler);
    signal(SIGTERM, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    

   //cria processo auth req man
    output("A Criar o Authorization Requests Manager");
    authReqManagerId = fork();
    if(authReqManagerId == 0){
        authReqManager();
        exit(0);
    }else if(authReqManagerId < 0){
        erro("Erro ao criar Authorization Request Manager");
    }
    
    //cria processo monitor engine
    output("A Criar Monitor Engine");
    monitorEngineId = fork();
    if(monitorEngineId == 0){
        monitorEngine();
        exit(0);
    }else if(monitorEngineId < 0){
        erro("Erro ao criar Monitor Engine");
    }
   
    //espera q os processos acabem
    output("A espera que todos os processos acabem");
    wait(NULL); //auth req
    wait(NULL); // monitor engine
   
    output("A acabar a simulation");
    hardclose();
    return 0;
}
