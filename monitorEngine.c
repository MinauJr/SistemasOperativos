// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "functions.h"

void * statsThread(void * arg){
    output("Stats Thread vai comecar");
    while(1){
        puts("Stats thread vai dormir");
        // wait 30 sec
        sleep(30);
        puts("Stats thread acordou");
        // backoffice user?
        if(shm->backofficePID != 0){
            puts("Stats thread a enviar as stats");
            statsSend("Update periodico");
        } 
    }
    output("Stats Thread a sair");
    pthread_exit(NULL);
}

int monitorEngine() {
    output("A inicializar Monitor Engine\n");
    puts("A Criar stats thread");
    pthread_create(&tid_stats, NULL, statsThread, NULL);
    

    
    pthread_join(tid_stats, NULL);
    output("Monitor Engine a acabar\n");
    return 0;
}