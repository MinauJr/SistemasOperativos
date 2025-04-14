// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "functions.h"

void regUser(int pid, int plafond){
    puts("A registrar um user");
    // Lock  shm
    sem_wait(sharedMemSem);
    puts("A verificar se o new user pode ser registrado");
    // verificacao
    if(shm->numRegUsers == configu.mobile_users){
       output("User nao pode ser registrado pois o nr maximo ja foi atingido.!"); //max mb_users
        sem_post(sharedMemSem);
        return;
    }
    // ultimo elem da fila
    mobile_user * current = &shm->mb_users[shm->numRegUsers];
    printf("A inicializar a info do new userr %d com pid %d\n", shm->numRegUsers, pid);
    // Add info do new user
    current->dataConsumed = 0;
    current->pid = pid;
    current->plafond = plafond;
    shm->numRegUsers++;
    // Unlock shm
    sem_post(sharedMemSem);
    output("New user registrado");
}

void procReq(int pid, char *service, int amount){
    puts("A processar o request");
    // Lock shm
    sem_wait(sharedMemSem);
    // verificacao para ver o u user existe
    mobile_user * current = shm->mb_users;
    int i;
    printf("A verificar se o user existe dos %d users existentes\n", shm->numRegUsers);
    for(i = 0; i < shm->numRegUsers; i++){
        printf("User %d com pid %d\n", i, current->pid);
        if(current->pid == pid) break;
        current++;
    }
    //se nao ouver nenhum user
    if(i == shm->numRegUsers){
        output("Nenhum user encontrado com esse ID");
        sem_post(sharedMemSem);
        return;
    }
    puts("A verificar se o user tem plafond suficiente");
    // plafond
    if(current->dataConsumed + amount > current->plafond){
        output("O user nao tem plafond sufuciente para fazer o req");
        sem_post(sharedMemSem);
        return;
    }
    puts("A verificar o tipo de pedido");
    // service
    for(int i = 0; i < 3; i++){
        if(strcmp(service, shm->statss[i].service) == 0){
            shm->statss[i].auth_reqs++;
            shm->statss[i].total_data += amount;
        }
    }
    // atualizar o dataconsumed de cada user
    current->dataConsumed += amount;
    // Unlock shm
    sem_post(sharedMemSem);
}

void authEngine(auth_engine * currentAuthEngine){
    output("Auth Engine iniciado");
    while (1) {
        char buf[50], temp[50], *token, *pid;
        puts("A espera pelo req do unnamed pipe");
        // a espera de Receiver req do unnamed pipe
        read(currentAuthEngine->unnammedPipeFd[0], &buf, 50);
        printf("O request: %s\n", buf);
        // Tornar este  Authorization Engine unavailable
        sem_wait(&currentAuthEngine->mut);
        //verificar se o req e do backoffice user
        strcpy(temp, buf);
        pid = strtok(temp, "#");
        puts("A verificar se o request veio do backoffice");
        // se vier....
        // pid#request
        if(atoi(pid) == shm->backofficePID){
            puts("O pedido veio do backoffice");
            // data_stats request
            token = strtok(NULL, " #\n\r\0");
            if(strcmp(token, "data_stats") == 0){
                puts("O pedido : data_stats");
                statsSend("Pedido de estatisticas");
            } 
            // reset
            else {
                puts("Pedido de reset");
                statsReset();
            } 
        }
        // pedidos do user?
        else {
            puts("A verificar se é registro inicial");
            // Check if it's a initial registration
            // pid#plafond
            token = strtok(NULL, " #\n\r\0");
            if(isdigit(token[0])){
                puts("Registro Inicial");
                regUser(atoi(pid), atoi(token));
            }
            // verifica se é authorization request
            else {
                puts("Authorization request");
                // pid#SERVICE#amount
                char *amount;
                amount = strtok(NULL, " #\n\r\0");
                procReq(atoi(pid), token, atoi(amount));
            }
            puts("Authorization Engine a dormir");
            // Apenas user requests usam AUTH_PROC_TIME
            usleep(configu.auth_proc_time * 1000); // milisegundos
            puts("Authorization Engine acordou ");
            puts("Authorization Engine enviou signal para o Monitor Engine");
            
            //AQUI ESTAVAM OS ALERTAS
        }
        // Torna este Authorization Engine available
        sem_post(&currentAuthEngine->mut);
    }
    output("O Authorization Engine acabou");
    pthread_exit(NULL);
}

void deleteAuthorizationEngine(){
    puts("A remover um Authorization Engine");
    pthread_mutex_lock(&authorization_engines_mutex);
    // fim da fila
    auth_engine * currentAuthEngine = &auth_engines[numAuthEngines - 1];
    // Wait pelo fim e o processamento
    sem_wait(&currentAuthEngine->mut);
    kill(currentAuthEngine->pid, SIGINT);
    close(currentAuthEngine->unnammedPipeFd[0]);
    close(currentAuthEngine->unnammedPipeFd[1]);
    sem_post(&currentAuthEngine->mut);
    sem_destroy(&currentAuthEngine->mut);
    printf("A remover o Authorization Engine %d\n", numAuthEngines - 1);
    numAuthEngines--;

    pthread_mutex_unlock(&authorization_engines_mutex);
    output("Um Authorization Engine foi removido");
}

void createAuthorizationEngine(){
    puts("A criar um Authorization Engine");
    pthread_mutex_lock(&authorization_engines_mutex);
    // verifica se outro auth engine pode ser criado
    puts("A verificar se o numAuthEngines atingiu o nr maximo");
    if(numAuthEngines == configu.auth_servers_max){
        pthread_mutex_unlock(&authorization_engines_mutex);
        output("Erro ao criar outro Authorization Engine porque o numero maximo foi atingido");
        return;
    }
    // fim da lista
    auth_engine * currentAuthEngine = &auth_engines[numAuthEngines];
    printf("A iniciar as variaveis do Authorization Engine %d\n", numAuthEngines);
    // Inicia as variaveis do Authorization Engine
    pipe(currentAuthEngine->unnammedPipeFd);
    sem_init(&currentAuthEngine->mut, 1, 1);
    currentAuthEngine->pid = fork();
    if(currentAuthEngine->pid == 0){
        authEngine(currentAuthEngine);
        exit(0);
    }
    // Atualiza numAuthEngines
    numAuthEngines++;
    pthread_mutex_unlock(&authorization_engines_mutex);
    output("Um novo Authorization Engine foi criado");
}