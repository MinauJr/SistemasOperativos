// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "functions.h"

//META 2

void *receiverThread(void *arg) {
    output("Receiver Thread inicializada");
    fd_set read_set;
    char msg[50];
    while(1){
        memset(msg, 0, 50);
        // Wait  messages
        FD_ZERO(&read_set);//limpa
        //quero ouvir estes fds:
        FD_SET(nammedPipeFd[0], &read_set); //back_pipe
        FD_SET(nammedPipeFd[1], &read_set);// user_pipe
        puts("Reciever a espera de messages");
        //espera passiva enquanto n houver alteracao no pipe, ultimo fd + 1, ordem interessa , tamos a ouvir ate ao "3"
        if (select(nammedPipeFd[1] + 1, &read_set, NULL, NULL, NULL) > 0) {
            puts("Reciever recebeu uma message");
            pthread_mutex_lock(&queues_mutex);
            // Verifica se veio do backoffice
            if(FD_ISSET(nammedPipeFd[0], &read_set)){ //verifica se a atualizao foi neste pipe
                puts("Receiver recebeu uma message do backoffice");
                read(nammedPipeFd[0], &msg, 50);
                //1 vez do backoffice_user a enviar uma message, guarda o pid
                if(shm->backofficePID == 0){
                    // Atualiza o backofficePID
                    // pid#data_stats || pid#reset
                    sem_wait(sharedMemSem);
                    char * pid, temp[50];
                    // Get pid
                    strcpy(temp, msg);
                    pid = strtok(temp, "#");
                    shm->backofficePID = atoi(pid);
                    output("Recebi a 1 mensagem do backoffice_user");
                    sem_post(sharedMemSem);
                }
                // addiciona ao Other_Services_Queue
                if(OtherServicesQueue.currentElements < configu.queue_pos){
                    puts("Message adicionada do backoffice para Other_Services_Queue");
                    Enqueue(&OtherServicesQueue, msg);
                } 
                // cheia
                else {
                    output("Req apagado porque o Other_Services_Queue esta cheia");
                } 
                memset(msg, 0, 50);
            }
            // verifica se veio do user
            if(FD_ISSET(nammedPipeFd[1], &read_set)){ //verifica se a atualizao foi neste pipe
                puts("Recebi a message do user");
                read(nammedPipeFd[1], &msg, 50);
                // verifica se e video authorization "123#VIDEO#123". se sim -> Video_Streaming_Queue
                char *token, temp[50];
                strcpy(temp, msg);
                token = strtok(temp, "#");
                token = strtok(NULL, "#\0\n\r ");
                if(strcmp(token, "VIDEO") == 0){
                    // adiciona a Video_Streaming_Queue
                    if(VideoStreamingQueue.currentElements < configu.queue_pos){
                        Enqueue(&VideoStreamingQueue, msg);
                        puts("Message do mobile user adicionada a Video_Streaming_Queue");
                    } 
                    // cheia
                    else {
                        output("Req apagado porque a Video_Streaming_Queue esta cheia");
                    }
                }
                else {
                    // Other_Services_Queue
                    if(OtherServicesQueue.currentElements < configu.queue_pos){
                        Enqueue(&OtherServicesQueue, msg);
                        puts("Message do mobile user adicionada a Other_Services_Queue");
                    } 
                    // cheia
                    else {
                        output("Req apagado porque a Other_Services_Queue esta cheia");
                    }
                }              
            }
            puts("Enviando signal ao Sender e Authorization Requests Manager");
            pthread_cond_broadcast(&cond_queues);// manda sinal
            pthread_mutex_unlock(&queues_mutex);
        }
    }
    output("Receiver Thread saindo");
    pthread_exit(NULL);
}

void *senderThread(void *arg) {
    output("Sender Thread inicializada");
    int currentAuthEngineIndex = 0;
    while(1){
        pthread_mutex_lock(&queues_mutex);
        //while n ha elementos na lista
        while(VideoStreamingQueue.currentElements == 0 && OtherServicesQueue.currentElements == 0){
            puts("Sender a espera de um signal");
            //ficamos aqui a espera ate receber o broadcast de cima
            pthread_cond_wait(&cond_queues, &queues_mutex);
            puts("Sender recebeu um signal");
        }
        time_t currentTime = time(NULL);
        // Process video requests em 1
        if(VideoStreamingQueue.currentElements > 0){
            puts("Recebendo o node da video queue");
            node * n = Dequeue(&VideoStreamingQueue);
            puts("Verificando se o node da video queue foi timeout");
            // Check if it has timed out
            if((double) (currentTime - n->timeoutCheck) > (double) (configu.max_video_wait / 1000.0)){
                output("Req apagado da Video_Streaming_Queue porque o pedido foi timeout");
            } 
            // Enviar o req para um Authorization Engine disponivel
            else {
                output("Enviando o req da video queue para um Auth Engine");
                //mutex para a lista de auth engines
                pthread_mutex_lock(&authorization_engines_mutex);
                auth_engine * current = &auth_engines[numAuthEngines - 1];// vai buscar o fim da fila
                // Incrementa  e verifica se é mais do q numAuthEngines
                currentAuthEngineIndex++;
                if(currentAuthEngineIndex >= numAuthEngines) currentAuthEngineIndex = 0;
                pthread_mutex_unlock(&authorization_engines_mutex);

                puts("Authorization Engine encontrado tou a espera que fique disponivel");
                // Wait 
                sem_wait(&current->mut);
                printf("Authorization Engine %d disponivel, a enviar o req\n", numAuthEngines - 1);   //nao era necessario semaforo
                // Send req
                write(current->unnammedPipeFd[1], &n->msg, 50);
                sem_post(&current->mut);
                puts("Req do video queue enviado para o Authorization Engine");
            }
        } 
        //others 
        else {
            puts("Recebendo um node da other queue");
            node * n = Dequeue(&OtherServicesQueue);
            puts("Verificando se o node foi timeout");
            // Check pelo timed out
            if((double) (currentTime - n->timeoutCheck) > (double) (configu.max_others_wait / 1000.0)){
                output("Req apagado da Other_Services_Queue porque foi timed out");
            } 
            // Enviando req para um available
            else {
                output("Enviando o req da other queue para um auth Engine");
                pthread_mutex_lock(&authorization_engines_mutex);
                auth_engine * current = &auth_engines[numAuthEngines - 1];
                // Incrementa  e verifica se é mais do q numAuthEngines
                currentAuthEngineIndex++;
                if(currentAuthEngineIndex >= numAuthEngines) currentAuthEngineIndex = 0;
                pthread_mutex_unlock(&authorization_engines_mutex);
                puts("Authorization Engine encontrado, tou a espera que fique disponivel");
                // Wait for it to be available
                sem_wait(&current->mut);
                printf("Authorization Engine %d disponivel, a enviar o req\n", numAuthEngines - 1);
                // Send it the request
                write(current->unnammedPipeFd[1], &n->msg, 50);
                sem_post(&current->mut);
                puts("Req do video queue enviado para o Authorization Engine");
            }
        }
        pthread_mutex_unlock(&queues_mutex);
        puts("Acabei de enviar o req para o Auth Engine");
    }
    output("Sender Thread saindo");
    pthread_exit(NULL);
}

int authReqManager() {
    output("A comecar o Authorization Requests Manager");

    // Criar pipes
    // BACK_PIPE
    output("A Criar e abrir o named pipe BACK_PIPE");
    if ((mkfifo("BACK_PIPE", O_CREAT|O_EXCL|0600) < 0) && (errno != EEXIST))
    {
        erro("Erro ao criar o named pipe BACK_PIPE");
    }
    if ((nammedPipeFd[0] = open("BACK_PIPE", O_RDWR)) < 0)
    {
        erro("Erro ao abrir o named pipe BACK_PIPE");
    }
    // USER_PIPE 
    output("A Criar e abrir o named pipe USER_PIPE");
    if ((mkfifo("USER_PIPE", O_CREAT|O_EXCL|0600) < 0) && (errno != EEXIST))
    {
        erro("Erro ao criar o named pipe USER_PIPE");
    }
    if ((nammedPipeFd[1] = open("USER_PIPE", O_RDWR)) < 0)
    {
        erro("Erro ao abrir o named pipe USER_PIPE");
    }

    // Aux
    int videoQueueFull = 0;
    int otherQueueFull = 0;
    puts("Lock aos mutextes das queues");
    pthread_mutex_lock(&queues_mutex);
    // Criar a liste da Authorization Engines
    puts("A alocar espaco para os Auth Engines");
    auth_engines = malloc(sizeof(auth_engine) * configu.auth_servers_max);
    if(auth_engines == NULL){
        erro("Erro a alocar espaco para os authorization engines");
    }
    numAuthEngines = 0;
    // Criar e comecar o 1 Authorization Engine
    puts("A Criar o 1 Authorization Engine");
    createAuthorizationEngine();
    pthread_mutex_unlock(&queues_mutex);

    output("A Criar as Threads Sender e Receiver");
    pthread_create(&tid_receiver, NULL, receiverThread, NULL);
    pthread_create(&tid_sender, NULL, senderThread, NULL);



    // Criar Authorization Engines 
    while(1){
        pthread_mutex_lock(&queues_mutex);
        //enquanto as filas n tao cheias && se ja tiver 1 extra criado e current element maior q a metade , se alguma falhar, ele sai
        while((VideoStreamingQueue.currentElements != configu.queue_pos || 
            (videoQueueFull > 0 && VideoStreamingQueue.currentElements > configu.queue_pos * 0.5)) && 
            (OtherServicesQueue.currentElements != configu.queue_pos || 
            (otherQueueFull > 0 && OtherServicesQueue.currentElements > configu.queue_pos * 0.5))) {

            puts("Aux a espera do signal");
            pthread_cond_wait(&cond_queues, &queues_mutex);
            puts("Aux recebeu o signal");
        }

        puts("Verifica se o video queue esta cheia");
        // cheia
        if(VideoStreamingQueue.currentElements == configu.queue_pos){
            puts("Video queue cheia");
            // Increment 
            videoQueueFull++;
            // Cria Authorization Engine
            createAuthorizationEngine();
        }
        puts("Verifica se o extra Authorization Engine pode ser removido");
        // If there's an extra Authorization Engine and the queue is at 50% capacity
        if(videoQueueFull > 0 && VideoStreamingQueue.currentElements <= configu.queue_pos * 0.5){
            puts("O Authorization Engine extra pode ser removido");
            // Decrementa
            videoQueueFull--;
           
            deleteAuthorizationEngine();
        }
        puts("Verifica se a other queue esta cheia");
        if(OtherServicesQueue.currentElements == configu.queue_pos){
            puts("Other queue esta cheia");
            // Incrementa
            otherQueueFull++;
            // Criar o novo Authorization Engine
            createAuthorizationEngine();
        }
        puts("Verifica se o Auth Engine exxtra pode ser removido");
        // If there's an extra Authorization Engine and the queue is at 50% capacity
        if(otherQueueFull > 0 && OtherServicesQueue.currentElements <= configu.queue_pos * 0.5){
            puts("O Authorization Engine extra pode ser removido");
            // Decrementa
            otherQueueFull--;
            
            deleteAuthorizationEngine();
        }
        pthread_mutex_unlock(&queues_mutex);
    }

    output("A espera das Threads");
    pthread_join(tid_receiver, NULL);
    pthread_join(tid_sender, NULL);

    output("Authorization Requests Manager chegou ao fim");

    return 0;
}

