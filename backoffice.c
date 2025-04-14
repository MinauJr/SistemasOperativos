// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "backoffice.h"

int main() {
    printf("BackOffice User started.\n");

    printf("Abrir o named pipe\n"); //BACK_PIPE
    // Abrir o named pipe 
    if ((backPipeFd = open("BACK_PIPE", O_WRONLY)) < 0){
        errobo("Erro ao abrir back pipe para escrita");
    }

    printf("A abrir a message queue\n");
    // Abrir a message queue
    msgId = msgget(ID_MSGQ, 0);
    if(msgId == -1){
        errobo("Error ao abrir a message queue");
    }

    // Signal
    signal(SIGINT, terHandlerbo);
    signal(SIGTERM, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    printf("A Criar a receiver thread\n");
    // Criar a thread para recieve stats
    pthread_create(&tid_stats, NULL, statsRec, NULL);

    
    while(1){
        int option;
        printf("------------------------------------------------------------------\n");
        printf("Escolha a opcao pretendida:\n");
        printf("|0. Sair\n|1. Estatisticas\n|2. Reset estatisticas\n");
        printf("------------------------------------------------------------------\n");
        scanf("%d", &option);
        // Sair
        if(option == 0){
            break;
        }
        else if(option == 1) {
            // Criar a data_stats message
            char dataStatsMsg[50];
            sprintf(dataStatsMsg, "%d#data_stats", getpid());
            // Enviar a data_stats message
            write(backPipeFd, &dataStatsMsg, 50);
            printf("Pedido das Data Stats  enviado\n");
        }
        else if(option == 2){
            // Criar reset message
            char resetMsg[50];
            sprintf(resetMsg, "%d#reset", getpid());
            // Enviar a reset message
            write(backPipeFd, &resetMsg, 50);
            printf("Pedido de Reset enviado\n");
        }
        else {
            printf("Operacao desconhecida\n");
        }
    }
    closebo();
    printf("BackOffice User ending.\n");
    return 0;
}