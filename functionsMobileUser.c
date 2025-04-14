// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "mobile.h"

//hardclose
void closemb(){
    for(int i = 0; i < 3; i++){
        pthread_cancel(reqsmb[i]);
    }
    pthread_mutex_destroy(&mutexmb);
    close(userPipeFd);
    msgctl(ID_MSGQ, IPC_RMID, NULL);
    printf("Adeus user %d\n", getpid());
    exit(0);
}

//signal
void terHandler(int signum){
    printf("Sinal recebido (^C)!\n");
    closemb();
}

//erro para chamar o hardclose
void erromb(char * str){
    printf("%s\n", str);
    closemb();
}

//threads
void * videoReq(void * inter){
    // Get o intervalo de video

    //obtém o valor int armazenado no endereço de memória apontado por inter.
    int videoInter = *((int *) inter);
    char videoMsg[50];
    // Criar a video message
    sprintf(videoMsg, "%d#VIDEO#%d", getpid(), dataReservar);
    while(1){
        //Espera para enviar a next message
        sleep(videoInter);
        if(numPedidosMax <= 0){
            break;
        }
        // Lock  mutexmb
        pthread_mutex_lock(&mutexmb);

        // Send video request
        if (write(userPipeFd, &videoMsg, 50) == -1) {
            erromb("Erro ao escrever o pedido de vídeo");
        }
        puts("Envia VIDEO req\n");
        // Decrementar
        numPedidosMax--;
        // Unlock  mutexmb
        pthread_mutex_unlock(&mutexmb);
    }
    msg msg;
    msg.msgtype = getpid();
    strcpy(msg.msg, "Acabado");
    msgsnd(msgId, &msg, sizeof(msg), 0);
    pthread_exit(NULL);
}

void * musicReq(void * inter){
      // Get o intervalo de music
    int musicInter = *((int *) inter);
    char musicMsg[50];
    // criar a music message
    sprintf(musicMsg, "%d#MUSIC#%d", getpid(), dataReservar);
    while(1){
        // Espera para enviar a next message
        sleep(musicInter);      

        if(numPedidosMax <= 0){
            
            break;
        }
        pthread_mutex_lock(&mutexmb);

        // music req
        if (write(userPipeFd, &musicMsg, 50) == -1) {
           erromb("Erro ao escrever o pedido de vídeo");
        }
        puts("Envia MUSIC req\n");
        // Decrementar
        numPedidosMax--; 
        //mutexmb unlock
        pthread_mutex_unlock(&mutexmb);
    }
    msg msg;
    msg.msgtype = getpid();
    strcpy(msg.msg, "Acabado");
    msgsnd(msgId, &msg, sizeof(msg), 0);
    pthread_exit(NULL);
}

void * socialReq(void * inter){
    //social interval
    int socialInter = *((int *) inter);
    char socialMsg[50];
    // criar a social message
    sprintf(socialMsg, "%d#SOCIAL#%d", getpid(), dataReservar);
    while(1){
        // Esperar pela next message
        sleep(socialInter);
        if(numPedidosMax <= 0){
            break;
        }
        pthread_mutex_lock(&mutexmb);
        // Envia social req
        if (write(userPipeFd, &socialMsg, 50) == -1) {
           erromb("Erro ao escrever o pedido de vídeo");
        }  
        puts("Envia SOCIAL req\n");
        // Decrementar
        numPedidosMax--; 
        pthread_mutex_unlock(&mutexmb);
    }
    msg msg;
    msg.msgtype = getpid();
    strcpy(msg.msg, "Acabado");
    msgsnd(msgId, &msg, sizeof(msg), 0);
    pthread_exit(NULL);
}


