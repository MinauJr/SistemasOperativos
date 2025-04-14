// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "backoffice.h"


void closebo(){
    pthread_cancel(tid_stats);
    close(backPipeFd);
    msgctl(ID_MSGQ, IPC_RMID, NULL);
    printf("Adeus user %d\n", getpid());
    exit(0);
}

void terHandlerbo(int signum){
    printf("Sinal recebido (^C)!\n");
    closebo();
}

void errobo(char * str){
    printf("%s\n", str);
    closebo();
}

void * statsRec(void * arg){
    while(1){
        // Receber as stats message
        msg mensg;
        if(msgrcv(msgId, &mensg, sizeof(msg), getpid(), 0) == -1){
            errobo("Erro recebendo a message da message queue");
        }
        printf("------------------------------------------------------------------\n");
        puts(mensg.msg);
        printf("|Service|\t\t|Data Total\t\t|Auth Reqs|\n");
        // VIDEO
        printf("|VIDEO:\t%d\t\t%d\n", mensg.totalDataVideo, mensg.authReqsVideo);
        // MUSIC
        printf("|MUSIC:\t%d\t\t%d\n", mensg.totalDataMusic, mensg.authReqsMusic);
        // SOCIAL
        printf("|SOCIAL:%d\t\t%d\n", mensg.totalDataSocial, mensg.authReqsSocial);
        printf("------------------------------------------------------------------\n");
    }
    pthread_exit(NULL);
}
