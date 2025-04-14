// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601

#include "mobile.h"


int main(int argc, char *argv[]) {
    //Verifica o nr de args
    if (argc != 7) {
        printf("Usage: mobile_user <plafond inicial> <número máximo pedidos de autorização> <intervalo VIDEO> <intervalo MUSIC> <intervalo SOCIAL> <dados a reservar>\n");
        exit(0);
    }
    
    //atoi para int
    plafondInicial = atoi(argv[1]);
    numPedidosMax = atoi(argv[2]);
    video = atoi(argv[3]);
    music = atoi(argv[4]);
    social = atoi(argv[5]);
    dataReservar = atoi(argv[6]);

    //Verifica se os valores são válidos
    if (plafondInicial <= 0 || numPedidosMax <= 0 || video < 0 || music < 0 || social < 0 || dataReservar < 0) {
        printf( "Argumentos inválidos fornecidos.\n");
        exit(0);
    }

    //Printa os valores para verificar
    printf("Mobile User Iniciado com os parametros:\n");
    printf("Plafond Inicial: %d\n", plafondInicial);
    printf("Número máximo de Pedidos de autorização: %d\n", numPedidosMax);
    printf("Intervalos: Video=%d, Música=%d, Social=%d\n", video, music, social);
    printf("Dados a Reservar por Pedido: %d\n", dataReservar);

    //META 2

    //SIGINT
    signal(SIGINT, terHandler);
    signal(SIGTERM, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    puts("A abrir USER_PIPE\n");
    // abrir o named pipe 
    if ((userPipeFd = open("USER_PIPE", O_WRONLY)) < 0){
        erromb("Erro ao abrir o user pipe para escrita");
    }
    
    puts("A abrir Message Queue\n");
    // Abrir message queue
    msgId = msgget(ID_MSGQ, 0);
    if(msgId == -1){
        erromb("Erro ao abrir a message queue");
    }

    mutexmb = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    puts("A enviar mensagem de registo\n");
    // criar a registration message
    char pipeMsg[50];
    sprintf(pipeMsg, "%d#%d", getpid(), plafondInicial);
    //Enviar a registration message
    write(userPipeFd, &pipeMsg, 50);

    puts("A criar Threads\n");
    // Threads reqsmb
    // video
    pthread_create(&reqsmb[0], NULL, videoReq, &video);
    // music
    pthread_create(&reqsmb[1], NULL, musicReq, &music);
    // social
    pthread_create(&reqsmb[2], NULL, socialReq, &social);

    // Queue Reader
    while(numPedidosMax > 0){
        msg msg;

        printf("A esperar por mensagens com id %d\n", getpid());
        // Read the messages from the message queue associated with my pid
        if(msgrcv(msgId, &msg, sizeof(msg), getpid(), 0) == -1){ 
            erromb("Erro ao receber mensagem da message queue");
        }
        // Print 
        printf("%s\n", msg.msg);
        
        if(strncmp(msg.msg, "100", sizeof("100")) == 0){
            break;
        }
    }

    // Wait threads
    for(int i = 0; i < 3; i++){
        pthread_join(reqsmb[i], NULL);
    }

    return 0;
}