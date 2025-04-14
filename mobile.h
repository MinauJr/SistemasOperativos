// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa   2021218601

#ifndef MOBILE_H
#define MOBILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/msg.h>

#define ID_MSGQ 500

int msgId;
int userPipeFd;
int plafondInicial, numPedidosMax, video, music, social, dataReservar;
pthread_t reqsmb[3];
pthread_mutex_t mutexmb;

typedef struct {
    long msgtype;
    char msg[100];
    int totalDataVideo;
    int totalDataMusic;
    int totalDataSocial;
    int authReqsVideo;
    int authReqsMusic;
    int authReqsSocial;
} msg;



void closemb();
void terHandler(int signum);
void erromb(char * str);
void * videoReq(void * inter);
void * musicReq(void * inter);
void * socialReq(void * inter);

#endif
