#include "user_process.h"
#include <signal.h>


int isTerminated() {
	return generateRandomNumber(GET_RANDOM_PERCENTAGE) <= TERMINATE_PCT;
}

int isRequesting() {
	return generateRandomNumber(GET_RANDOM_PERCENTAGE) <= REQUEST_PCT;
}

int isReadOrWrite() {
	int is_read = generateRandomNumber(GET_RANDOM_PERCENTAGE);

	return is_read > 30 ? READ_FLAG : WRITE_FLAG;
}

//first memory request scheme
int generateRandomNumber(int flag) {
	switch (flag)
	{
		case GET_RANDOM_PERCENTAGE:
			return rand() % 100 + 1;
			break;
		case PAGE_NUMBER_RANDOM:
			return rand() % MAX_MEMORY;
			break;
		case PAGE_OFFSET_RANDOM:
			return rand() % 1024; // 0 to 1023
			break;
		default:
			return 0;
			break;
	}
}

//second memory request scheme
double* buildWeightedArray() {
	double* weighted_array = (double *)malloc(sizeof(double) * MAX_MEMORY);
	double initial_array[MAX_MEMORY];

	int i;
	for (i = 0; i < MAX_MEMORY; i++) {
		initial_array[i] = (double) 1 / (i + 1);
	}

	double next_number = 0;
	for (i = 0; i < MAX_MEMORY; i++) {
		next_number += initial_array[i];
		weighted_array[i] = next_number;
	}

	return weighted_array;
}

void interrupt() {
	printf("\nInterrupt, so killing all the processes, message queues, and shm\n");
	terminate();
}

void terminate() {
	key_t sysclock_key;
	int sysclock_id;
	key_t oss_msgqueue_key;
	int oss_msgqueue_id;
	key_t user_msgqueue_key;
	int user_msgqueue_id;

	sysclock_key = ftok("/tmp", 'A');
	oss_msgqueue_key = ftok("/tmp", 'B');
	user_msgqueue_key = ftok("/tmp", 'C');

    sysclock_id = shmget ( sysclock_key, sizeof(unsigned int), 0777 | IPC_CREAT );
    oss_msgqueue_id = msgget(oss_msgqueue_key, 0666 | IPC_CREAT);
    user_msgqueue_id = msgget(user_msgqueue_key, 0666 | IPC_CREAT);

	msgctl(oss_msgqueue_id, IPC_RMID,NULL);
	msgctl(user_msgqueue_id, IPC_RMID,NULL);
    shmctl(sysclock_id, IPC_RMID,NULL);
    kill(0, SIGKILL);
}

int main(int argc, char** argv) {
	srand(time(NULL) + getpid());
	char perrorOutput[100];
	int current_process_id = atoi(argv[1]);
	int child_proc_mem_access_way = atoi(argv[2]);
	int mem_ref_cnt = 0;

	double* weighted_array;
	int last_number_in_weight_arr;

	if (child_proc_mem_access_way == WEIGHTED_MEMORY_ACCESS) {
		weighted_array = buildWeightedArray();
		int last_number_in_weight_arr = (int) (weighted_array[MAX_MEMORY-1] * 1000 ); // multiply 1000 so I don't have to worry about float
	}

	key_t sysclock_key;
	int sysclock_id;
	system_clock* sysclock;
	key_t oss_msgqueue_key;
	int oss_msgqueue_id;
	key_t user_msgqueue_key;
	int user_msgqueue_id;
	message msg;
	int current_resource = 0;
	int page_number;
	int page_offset;

	sysclock_key = ftok("/tmp", 'A');
	oss_msgqueue_key = ftok("/tmp", 'B');
	user_msgqueue_key = ftok("/tmp", 'C');

	if ( sysclock_key == (key_t) -1 || oss_msgqueue_key == (key_t) -1 || user_msgqueue_key == (key_t) -1 ) {
		printf("Error during ftok\n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
	}

	sysclock_id = shmget ( sysclock_key, sizeof(unsigned int), 0777 | IPC_CREAT );
    oss_msgqueue_id = msgget(oss_msgqueue_key, 0666 | IPC_CREAT);
    user_msgqueue_id = msgget(user_msgqueue_key, 0666 | IPC_CREAT);

	if ( sysclock_id == -1 || oss_msgqueue_id == -1 || user_msgqueue_id == -1 ) {
		printf("Error during get\n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
	}

    sysclock = shmat( sysclock_id, 0, 0 );
    if ( sysclock == (void *)-1  ) {
		printf("master: Error in shmat sysclock \n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
    }

	strcpy(perrorOutput, argv[0]);
	strcat(perrorOutput, ": Error: ");

	msg.mesg_type = PARENT_QUEUE_ADDRESS;
	msg.process_id = current_process_id;

	while (1) {
		mem_ref_cnt++;
		int rand_num_for_page = 0;
		int i;
		if ((mem_ref_cnt % 1000 == 0) && isTerminated())
		{
			// At random times every 1000 ± 100 memory references,
			// the user process will check whether it should terminate.
			// If so, all its memory should be returned to oss and oss should be informed of its termination.
			printf("Process P%d is Terminated\n", current_process_id);
			msg.action_flag = TERMINATE_FLAG;
			msgsnd(oss_msgqueue_id, &msg, sizeof(message), 0);
			return EXIT_SUCCESS;
		}

		if (child_proc_mem_access_way == NORMAL_MEMORY_ACCESS) {
			page_number = generateRandomNumber(PAGE_NUMBER_RANDOM);
		} else {
			// Weighted memory access
			rand_num_for_page = rand() % (last_number_in_weight_arr + 1);
			for (i = 0 ; i < MAX_MEMORY; i++) {

				int weight = (int) (weighted_array[i] * 1000); // Multiplying 1000 to make it simple to calculate decimal points
				if (weight > rand_num_for_page) {
					page_number = i;
					break;
				}
			}
		}

		// Set up offset
		// Multiply that page number by 1024 (or left shift by 10 bits)
		// and then add a random offset of from 0 to 1023 to get the actual memory address requested.
		page_offset = (page_number * 1024) + generateRandomNumber(PAGE_OFFSET_RANDOM);

		msg.address = page_offset;
		msg.action_flag = isReadOrWrite();
		msgsnd(oss_msgqueue_id, &msg, sizeof(message), 0);
		int msg_rcv = msgrcv(user_msgqueue_id, &msg, (sizeof(message)-sizeof(long)), current_process_id, IPC_NOWAIT);
	}

	return EXIT_SUCCESS;
}