// João Tomás Gomes Paiva 2021216669
// Salomé Ventura Costa 2021218601
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

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
#include <sys/stat.h>

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

//file
typedef struct {
    int mobile_users;
    int queue_pos;
    int auth_servers_max;
    int auth_proc_time;
    int max_video_wait;
    int max_others_wait;
} Config;

//mobile users
typedef struct {
    int pid;
    int plafond;
    int dataConsumed;
} mobile_user;

typedef struct {
	char service[10];
	int total_data;
	int auth_reqs;
} stats;

//memoria
typedef struct {
    stats statss[3];
    int numRegUsers;
	int backofficePID;
    mobile_user *mb_users;
} SharedData;

typedef struct Node{
    char msg[50];
	time_t timeoutCheck;
    struct Node *next;
} node;

typedef struct {
    node *head;
    node *tail;
    int currentElements;
} Queue;


typedef struct {
	pid_t pid;
	int unnammedPipeFd[2];
	sem_t mut;
} auth_engine;


FILE *logFile;
Config configu;
SharedData *shm;
int shm_id;
sem_t *sharedMemSem;
time_t currentTime;

Queue VideoStreamingQueue;
Queue OtherServicesQueue;

// Mutexes and conditional variables
pthread_mutex_t log_mutex;
pthread_mutex_t queues_mutex;
pthread_mutex_t authorization_engines_mutex;
pthread_cond_t cond_queues;


int nammedPipeFd[2];
int msgid;
pid_t monitorEngineId, authReqManagerId;
int videoInter,musicInter,socialInter;
pthread_t tid_receiver, tid_sender, tid_stats;
auth_engine * auth_engines;
int numAuthEngines;
int mainPID;

void readConfig(const char *filename);
void createSharedMemory();
void createSHMsemaph();
void createMsgQ();
void output(char *str);
void hardclose();
void erro(char * error);
void terHandler(int signum);

void init_queue(Queue *q);
void Enqueue(Queue *q, char msg[]);
node * Dequeue(Queue *q);

int authReqManager();
int monitorEngine();

void authEngine(auth_engine * currentAuthEngine);
void deleteAuthorizationEngine();
void createAuthorizationEngine();

void statsReset();
void statsSend(char * msg);
#endif