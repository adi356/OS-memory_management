#ifndef USER_PROCESS_H
#define USER_PROCESS_H

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <time.h>

#define BUFF_SZ	sizeof ( int )
#define PARENT_QUEUE_ADDRESS 999
#define MAX_MEMORY 32
#define TOTAL_SYSTEM_MEMORY 256
#define TERMINATE_CHECK_NS 250000000
#define TERMINATE_PCT 10
#define REQUEST_PCT 50

#define NUM_OF_RESOURCES 20
#define MAX_NUM_USER_PROC 18
#define TRUE  1
#define FALSE 0

#define GET_RANDOM_PERCENTAGE 0
#define PAGE_NUMBER_RANDOM 1
#define PAGE_OFFSET_RANDOM 2

#define TERMINATE_FLAG 1
#define READ_FLAG      2
#define WRITE_FLAG     3

#define NORMAL_MEMORY_ACCESS  0
#define WEIGHTED_MEMORY_ACCESS 1

typedef struct {
	unsigned int sec;
	unsigned int nano_sec;
} system_clock;

typedef struct {
    long mesg_type;
    int process_id;
    int action_flag;
    int mesg_granted;
    unsigned int address;
} message;

int  isTerminated();
int  isRequesting();
int  isReadOrWrite();
int  generateRandomNumber(int flag);
double* buildWeightedArray();
void interrupt();
void terminate();

#endif /* USER_PROCESS_H */