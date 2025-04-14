// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "functions.h"


void readConfig(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        erro("Erro ao abrir o config file");
    }

    //le os valores do ficheiro
    if (fscanf(file, "%d %d %d %d %d %d", &configu.mobile_users, &configu.queue_pos, 
               &configu.auth_servers_max, &configu.auth_proc_time, &configu.max_video_wait, 
               &configu.max_others_wait) != 6) {
        erro( "Configuration file tem o formato errado.\n");
        fclose(file);
    }

    //Valida os dados lidos
    if (configu.mobile_users <= 0 || configu.queue_pos <= 0 || configu.auth_servers_max <= 0 ||
        configu.auth_proc_time <= 0 || configu.max_video_wait <= 0 || configu.max_others_wait <= 0) {
        erro("Os Valores nao fazem sentido.\n");
        fclose(file);
    }
    fclose(file);
}

void createSharedMemory() {
   
    shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0700);
    if (shm_id < 1) erro("Erro ao criar a Shared Memory");
    shm= (SharedData *) shmat(shm_id, NULL, 0);
    if (shm < (SharedData *) 1) erro("Erro ao mapear a Shared Memory");

    // Iniciar as stats
    strcpy(shm->statss[0].service, "VIDEO");
    shm->statss[0].auth_reqs = 0;
    shm->statss[0].total_data = 0;

    strcpy(shm->statss[1].service, "MUSIC");
    shm->statss[1].auth_reqs = 0;
    shm->statss[1].total_data = 0;

    strcpy(shm->statss[2].service, "SOCIAL");
    shm->statss[2].auth_reqs = 0;
    shm->statss[2].total_data = 0;

    // Iniciar backoffice user
    shm->backofficePID = 0;
    shm->numRegUsers = 0;

    // Iniciar users
    shm->mb_users = malloc(configu.mobile_users * sizeof(mobile_user));
    if(shm->mb_users == NULL ){
        erro("Erro ao alocar espaco para os mobiles users");
    }
    
    /*
    shm->mb_users = ((void*)shm) + sizeof(shm->mb_users)+ sizeof(shm->tempor); // alocar memoria para o nr de mobile users
    //shm->tempor = ((void*)shm) + sizeof(shm->mb_users)+ sizeof(shm->tempor) + (configu.mobile_users * sizeof(mobile_user));

    if(shm->tempor== NULL ){
        erro("Error allocating space for TEMPOR");
    }
    */
}

void createSHMsemaph(){ 
    sem_unlink("sharedMemSem");
    sharedMemSem = sem_open("sharedMemSem", O_CREAT|O_EXCL, 0700, 1);
    if(sharedMemSem == SEM_FAILED){
        erro("Erro ao criar o shared memory semaphore");
    }
}

void createMsgQ(){    
    //message queue
    msgid = msgget(ID_MSGQ, IPC_CREAT | 0777);
    if(msgid == -1){
        erro("erro a criar queue");
    }
}


void output(char *str) {
    //da lock ao mutex para acesso ao ficheiro log e na consola
    pthread_mutex_lock(&log_mutex);

    //tempo atual
    time_t currentTime = time(NULL);
    struct tm *tm = localtime(&currentTime);
    
    if (tm != NULL) {
        //Escreve a mensagem no log com o tempo e id do processo
        fprintf(logFile, "%02d:%02d:%02d - [%d] %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, getpid(), str);
        //escreve imediatamente no log
        fflush(logFile);
        
        //escreve na consola
        fprintf(stdout, "%s\n", str);
    } else {
        fputs("Erro ao obter a hora local\n", stderr);
    }

    //da unlock ao mutex
    pthread_mutex_unlock(&log_mutex);
}

//termina tudo à forca
void hardclose(){
    output("Hard Closing tudo");
    
    output("A fechar as Threads");
    pthread_cancel(tid_receiver);
    pthread_cancel(tid_sender);
    pthread_cancel(tid_stats);

    output("A fechar todos os processos");
    //manda a todos os processos, o sig int
    kill(0, SIGINT); 
    output("A fechar os Authorization Engines");
    for(int i = 0; i < numAuthEngines; i++){
        deleteAuthorizationEngine();
    }
    free(auth_engines);
  
    output("A fechar os nammed pipes");
    close(nammedPipeFd[0]);
    close(nammedPipeFd[1]);
    unlink("BACK_PIPE");
    unlink("USER_PIPE");

    output("A Fechar os mutexes e as conditional variables");
    pthread_mutex_destroy(&queues_mutex);
    pthread_mutex_destroy(&authorization_engines_mutex);
    pthread_cond_destroy(&cond_queues);
    output("A fechar a Message Queue");
    msgctl(msgid, IPC_RMID, NULL);// message queue
    output("A fechar o Shared Memory Semaphor");
    sem_unlink("sharedMemSem");
    sem_close(sharedMemSem);
    output("A fechar a Shared Memory");
    free(shm->mb_users);
    shmdt(shm); // verificacao
    shmctl(shm_id, IPC_RMID, NULL); //libertar a memomry
    output("A fechar o log file");
    pthread_mutex_destroy(&log_mutex);
    fclose(logFile);
}

//para dar print de erro e terminar
void erro(char * error){
    output(error);
    hardclose();
    exit(0);
}


//para o sinal
void terHandler(int signum){ 
    if(getpid() == mainPID){
        erro("Sinal recebido (^C)!");
    }
    else {
        exit(0);
    }
}

void init_queue(Queue * q) {
    q->head = NULL;
    q->tail = NULL;
    q->currentElements = 0;
}

void Enqueue(Queue *q, char msg[]) {
    if (q->currentElements >= configu.queue_pos) {
        output("Fila está cheia\n");
        return;
    }
    q->currentElements += 1;
    // Criar no novo
    node *new_node = malloc(sizeof(node));
    strcpy(new_node->msg, msg);
    new_node->timeoutCheck = time(NULL);
    new_node->next = NULL;
    // Se não houver head
    if (q->head == NULL) {
        q->head = new_node;
    }
    else {
        q->tail->next = new_node;
    }
    q->tail = new_node;
}

node * Dequeue(Queue *q) {
    if (q->head == NULL) {
        output("Erro, tried to Dequeue while nothing in the queue\n");
        return NULL;
    }
    node *tmp = q->head;
    q->head = q->head->next;
    q->currentElements -= 1;
    return tmp;
}

void statsReset(){
    puts("Reset das Stats");
    // Lock shm
    sem_wait(sharedMemSem);
    // Reset all the stats
    for(int i = 0; i < 3; i++){
        shm->statss[i].auth_reqs = 0;
        shm->statss[i].total_data = 0;
    }
    // Unlock  shm
    sem_post(sharedMemSem);
    output("Stats resetadas");
}

void statsSend(char * message){
    // Lock  shm
    sem_wait(sharedMemSem);
    // all  stats
    msg stats;
    stats.msgtype = shm->backofficePID;
    strcpy(stats.msg, message);
    for(int i = 0; i < 3; i++){
        if(strcmp(shm->statss[i].service, "VIDEO") == 0){
            stats.authReqsVideo = shm->statss[i].auth_reqs;
            stats.totalDataVideo = shm->statss[i].total_data;
        }
        if(strcmp(shm->statss[i].service, "MUSIC") == 0){
            stats.authReqsMusic = shm->statss[i].auth_reqs;
            stats.totalDataMusic = shm->statss[i].total_data;
        }
        if(strcmp(shm->statss[i].service, "SOCIAL") == 0){
            stats.authReqsSocial = shm->statss[i].auth_reqs;
            stats.totalDataSocial = shm->statss[i].total_data;
        }
    }
    puts("Enviando as stats para o backoffice user");
    //  stats message
    msgsnd(msgid, &stats, sizeof(msg), 0); 
    // Unlock the shm
    sem_post(sharedMemSem);
    output("Stats enviadas para o backoffice user");
}