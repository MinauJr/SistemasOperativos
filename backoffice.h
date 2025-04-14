// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601
#ifndef BACKOFFICE_H
#define BACKOFFICE_H

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


int msgId;
int backPipeFd;
pthread_t tid_stats;

void closebo();
void terHandlerbo(int signum);
void errobo(char * str);
void * statsRec(void * arg);

#endif
