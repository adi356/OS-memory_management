#ifndef OSS_H
#define OSS_H

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <time.h>

#define NUM_OF_RESOURCES 20
#define MAX_NUM_USER_PROC 18
#define MAX_MEMORY 32
#define TOTAL_SYSTEM_MEMORY 256
#define ONE_SEC_IN_NANO 1000000000

#define PARENT_QUEUE_ADDRESS 999
#define BUFF_SZ	sizeof ( int )
#define TRUE  1
#define FALSE 0
#define PID_TAKEN 1
#define PID_AVAILBLE 0
#define REAL_TIME_PROC_PERCENTAGE 5

#define LOOP_ADVANCE_RANDOM 0
#define PROC_CREATE_NS_RANDOM 1
#define INIT_RESOURCE_NUMBER_RANDOM 2

#define TERMINATE_FLAG 1
#define READ_FLAG      2
#define WRITE_FLAG     3

#define REAL_TIME 0
#define USER_PROCESS 1

typedef struct {
	unsigned int sec;
	unsigned int nano_sec;
} system_clock;

typedef struct {
	unsigned int occupied;
	unsigned int reference_bit;
	unsigned int dirty_bit;
	unsigned int pid_own;
	unsigned int last_ref_sec;
	unsigned int last_ref_nano;
} frame;

typedef struct {
    long mesg_type;
    int process_id;
    int action_flag;
    int mesg_granted;
    unsigned int address;
} message;

void help();
int  checkToCreateNewProcess(int* available_pids, int total_process_generated, system_clock* sysclock, int last_proc_created_sec, int last_proc_created_ns);
int  isAfter(int current_sec, int current_ns, int compare_sec, int compare_ns);
int  generateRandomNumber(int flag);
void incrementSysClock(system_clock* sysclock, int seconds, int nano_seconds);
int* initPageTable();
int* initPidVector();
int* initActualPidVector();
int* initAllocFramesVector();
void timeout();
void interrupt();
void terminate();

#endif /* OSS_H */