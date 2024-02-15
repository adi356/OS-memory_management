#include "oss.h"
#include <signal.h>

static int is_timed_out = FALSE;

void help() {
	printf("There is no required inputs for this program. \n");
	printf("-h   : To see help message \n");
	printf("-m x : (0 or 1) To set the way child process will perform their memory access. \n");
}

// Checks conditions to create a new process based on various parameters.
// Returns TRUE if conditions are met, FALSE otherwise.
// Conditions include timeout and the number of processes generated.
int checkToCreateNewProcess(int *available_pids, int total_process_generated, system_clock* sysclock, int last_proc_created_sec, int last_proc_created_ns) {
    
	if (is_timed_out || total_process_generated > 40) {
		return FALSE;
	}

	int ns_to_seconds;

	int current_sec = sysclock->sec;
	int current_ns  = sysclock->nano_sec;
	//  fork a user process at random times (between 1 and 500 milliseconds of your logical clock)
	int create_process_at_sec = last_proc_created_sec;
	int create_process_at_ns  = last_proc_created_ns + generateRandomNumber(PROC_CREATE_NS_RANDOM);

	if (create_process_at_ns >= ONE_SEC_IN_NANO) {
		ns_to_seconds = 0;
		ns_to_seconds += create_process_at_ns / ONE_SEC_IN_NANO;
		create_process_at_ns -= ns_to_seconds * ONE_SEC_IN_NANO;
		create_process_at_sec += ns_to_seconds;
	}

	// Check the time
	if (!(current_sec == 0 && current_ns == 0) && !isAfter(current_sec, current_ns, create_process_at_sec, create_process_at_ns) ) {
		return FALSE;
	}

	return TRUE;
}

//compares two time values in seconds and nanoseconds returning true if current time after compared time
int isAfter(int current_sec, int current_ns, int compare_sec, int compare_ns) {
    
	return current_sec > compare_sec || (current_sec == compare_sec && current_ns > compare_ns);
}

// Generates random numbers based on the provided flag.
// The flag determines the type of random number to generate.
// It can generate loop advancement, process creation, and resource numbers.
int generateRandomNumber(int flag) {
    
	switch (flag)
	{
		case LOOP_ADVANCE_RANDOM:
			return rand() % 250000000 + 1;
			break;
		case PROC_CREATE_NS_RANDOM:
			return rand() % (500000000 + 1 - 1000000) + 1000000; // generate between 1000000 to 500000000
			break;
		case INIT_RESOURCE_NUMBER_RANDOM:
			return rand() % (10 + 1 - 1) + 1; // generate between 1 to 10
			break;

		default:
			return 0;
			break;
	}
}

// Increments the system clock by the specified number of seconds and nanoseconds.
// Handles the conversion of nanoseconds to seconds if necessary.
void incrementSysClock(system_clock* sysclock, int seconds, int nano_seconds) {
    
	int ns_to_seconds = 0;

	// Handling incoming nano seconds bigger than 1 sec in nano seconds
	if (nano_seconds >= ONE_SEC_IN_NANO) {
		ns_to_seconds += nano_seconds / ONE_SEC_IN_NANO;
		nano_seconds  -= ns_to_seconds * ONE_SEC_IN_NANO;
	}
	// Handling sysclock nano seconds bigger than 1 sec in nano seconds
	sysclock->sec += seconds + ns_to_seconds;
	sysclock->nano_sec += sysclock->nano_sec + nano_seconds;

	// recalculate current time if nano second is bigger than one sec
	ns_to_seconds = 0;
	if (sysclock->nano_sec >= ONE_SEC_IN_NANO) {
		ns_to_seconds += sysclock->nano_sec / ONE_SEC_IN_NANO;
		sysclock->sec += ns_to_seconds;
		sysclock->nano_sec -= (ns_to_seconds * ONE_SEC_IN_NANO);
	}
}

// Initializes the page table by allocating memory
// Returns a pointer to the allocated memory.
int* initPageTable() {
    
	int *page_table = (int *)malloc(sizeof(int) * TOTAL_SYSTEM_MEMORY);

	int i;
	for (i = 0; i < MAX_NUM_USER_PROC; i++) {
		page_table[i] = -1;
	}

	return page_table;
}

// Initializes the PID vector by allocating memory and setting initial values.
// Returns a pointer to the allocated memory.
int* initPidVector() {
    
	int *pids = (int *)malloc(sizeof(int) * MAX_NUM_USER_PROC);

	int i;
	for (i = 0; i < MAX_NUM_USER_PROC; i++) {
		pids[i] = PID_AVAILBLE;
	}
	return pids;
}

int* initActualPidVector() {
	int *actual_pids = (int *)malloc(sizeof(int) * MAX_NUM_USER_PROC);

	int i;
	for (i = 0; i < MAX_NUM_USER_PROC; i++)
	{
		actual_pids[i] = -1;
	}
	return actual_pids;
}

// allocate frames vector
int* initAllocFramesVector() {
	int *allocated_frames = (int *)malloc(sizeof(int) * TOTAL_SYSTEM_MEMORY);

	int i;
	for (i = 0; i < TOTAL_SYSTEM_MEMORY; i++) {
		allocated_frames[i] = -1;
	}

	return allocated_frames;
}

// Handles timeout signal.
// Cleans up resources and terminates processes.
void timeout() {
    
	printf("\nTime out, so killing all the processes, message queues, and shm\n");
	terminate();
}

// Handles interrupt signal.
// Cleans up resources and terminates processes.
void interrupt() {
    
	printf("\nInterrupt, so killing all the processes, message queues, and shm\n");
	terminate();
}

void terminate() {
    // Cleans up resources including message queues, shared memory, and terminates processes.
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
	srand(time(NULL));
	int maxTimeToRunProcess = 10;
	int child_proc_mem_access_way = 0;

	// Page Table
	int *page_table = initPageTable(); // each element is the frame number
	// Frame table
	// Use a bit vector to keep track of unallocated frames.
	int *allocated_frames = initAllocFramesVector();
	frame* frame_table = (frame *)malloc(sizeof(frame) * TOTAL_SYSTEM_MEMORY);

	char perrorOutput[100];
	char* fileName = "logfile";
	key_t sysclock_key;
	int sysclock_id;
	system_clock* sysclock;
	key_t oss_msgqueue_key;
	int oss_msgqueue_id;
	key_t user_msgqueue_key;
	int user_msgqueue_id;

	int line_number = 0;
	int total_process_generated = 0;
	int number_memory_access = 0;
	int total_page_fault = 0;

	// Bit vector to keep the process ids
	int* available_pids = initPidVector();
	pid_t* actual_pids = initActualPidVector();

	int last_proc_created_sec = 0;
	int last_proc_created_ns  = 0;

	strcpy(perrorOutput, argv[0]);
	strcat(perrorOutput, ": Error: ");

    //handle command line input
	int option;
	while ( (option = getopt(argc, argv, "hm:")) != -1 ) {
		switch(option)
		{
			case 'h':
				help();
				return EXIT_SUCCESS;
				break;
			case 'm':
				child_proc_mem_access_way = atoi(optarg);
				if (child_proc_mem_access_way > 1) {
					printf("m input cannot be bigger than 1. Setting it to 0\n");
					child_proc_mem_access_way = 0;
				}
				break;
			case '?':
				if (isprint (optopt))
				   fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				   fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return EXIT_FAILURE;
			default:
				help();
				return EXIT_SUCCESS;
		}
	}

	FILE* logfile = fopen(fileName, "w");

	sysclock_key = ftok("/tmp", 'A');
	oss_msgqueue_key = ftok("/tmp", 'B');
	user_msgqueue_key = ftok("/tmp", 'C');

	if ( sysclock_key == (key_t) -1 || oss_msgqueue_key == (key_t) -1 || user_msgqueue_key == (key_t) -1 ) {
	    printf("Error during ftok\n");
	    perror(perrorOutput);
	    exit(EXIT_FAILURE);
	}

	sysclock_id = shmget (sysclock_key , sizeof(system_clock), 0777 | IPC_CREAT );
    oss_msgqueue_id = msgget(oss_msgqueue_key, 0666 | IPC_CREAT);
    user_msgqueue_id = msgget(user_msgqueue_key, 0666 | IPC_CREAT);

    if ( sysclock_id == -1 || oss_msgqueue_id == -1 || user_msgqueue_id == -1 ) {
		printf("master: Error in shmget \n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
    }

    // Initialize system clock
    sysclock = shmat( sysclock_id, 0, 0 );
    if ( sysclock == (void *)-1  ) {
		printf("master: Error in shmat sysclock \n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
    }

    sysclock -> sec = 0;
    sysclock -> nano_sec = 0;

    if ( sysclock == (void *)-1  ) {
		printf("master: Error in shmat sysclock \n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
    }

    signal(SIGINT, interrupt);
    signal(SIGALRM, timeout);
    alarm(maxTimeToRunProcess); // Should terminate after 10 seconds

    printf("OSS: Processing... Will expire in 10 seconds\n");

    // while line number is less than 100000
	while (1)  {
		if (total_process_generated > 40) {
			printf("More than 40 process, terminating\n");
			terminate();
		}

		int pid_to_use = -1; // if -1, it won't be using this
		if (checkToCreateNewProcess(available_pids, total_process_generated, sysclock, last_proc_created_sec, last_proc_created_ns)) {
			// Check the pid vector and return the first available
			int i;
			for (i = 0; i < MAX_NUM_USER_PROC; i++) {
				if (available_pids[i] == PID_AVAILBLE) {
					printf("create a new process pid: %d\n", i);
					available_pids[i] = PID_TAKEN;
					pid_to_use = i;
					break;
				}
			}
		}

        //forking
		if (pid_to_use != -1) {
			pid_t childPid = fork();
			if (childPid == 0) {
				char arg1[10];
				char arg2[10];
				sprintf(arg1, "%d", pid_to_use);
				sprintf(arg2, "%d", child_proc_mem_access_way);
				execl("./user_proc", "./user_proc", arg1, arg2, (char*)0);
			} else {
				// Keeps the record of when the lastest process was created
				last_proc_created_sec = sysclock->sec;
				last_proc_created_ns  = sysclock->nano_sec;
				actual_pids[pid_to_use] = childPid;
				total_process_generated++;
			}
		} else {
			message msg;
			// receive from user_process
			int msg_rcv = msgrcv(oss_msgqueue_id, &msg, sizeof(message), PARENT_QUEUE_ADDRESS, IPC_NOWAIT);
			if (msg_rcv != -1) {
				number_memory_access++;
				if (msg.action_flag == TERMINATE_FLAG) {
					int pid_terminate = actual_pids[msg.process_id];
					available_pids[msg.process_id] = PID_AVAILBLE;
					actual_pids[msg.process_id] = -1;

					int i;
					for (i = 0; i < TOTAL_SYSTEM_MEMORY; i++) {
						if (frame_table[i].pid_own == msg.process_id) {
							frame_table[i].occupied = FALSE;
							frame_table[i].reference_bit = 0;
							frame_table[i].dirty_bit = 0;
							allocated_frames[i] = FALSE;
						}
					}
					waitpid(pid_terminate, NULL, 0);
				} else {
					// While a page is referenced, oss performs other tasks on the page table
					// as well such as updating the page reference, setting up dirty bit,
					// checking if the memory reference is valid
					// and whether the process has appropriate permissions on the frame, and so on.
					msg.mesg_type = msg.process_id;

					if (msg.action_flag == READ_FLAG) {
						fprintf(logfile, "Master: P%d requesting read of address %d at time %d:%d\n"
									, msg.process_id, msg.address, sysclock->sec, sysclock->nano_sec);
						fflush(logfile);
					} else {
						fprintf(logfile, "Master: P%d requesting write of address %d at time %d:%d\n"
									, msg.process_id, msg.address, sysclock->sec, sysclock->nano_sec);
						fflush(logfile);
					}

					// Each request for disk read/write takes about 14ms to be fulfilled
					incrementSysClock(sysclock, 0, 14000000);

					// Need to check page fault
					int page_number = msg.address / 1024;
					int frame_number = page_table[page_number];
					int frame_allocated = frame_number != -1 ? allocated_frames[frame_number] : -1;

					// if frame is not empty and not page fault
					if (frame_allocated != -1 && frame_table[frame_number].pid_own == msg.process_id) {
						// oss just increments the clock by 10 nanoseconds and sends a signal on the corresponding semaphore
						incrementSysClock(sysclock, 0, 10);
						if (msg.action_flag == READ_FLAG) {
							fprintf(logfile, "Master: Address %d in frame %d, giving data to P%d at time %d:%d\n"
										, msg.address, frame_number, msg.process_id, sysclock->sec, sysclock->nano_sec);
							fflush(logfile);
							int i;
							// If there is a reference made, reset all the reference bit
							for (i = 0; i < TOTAL_SYSTEM_MEMORY; i++) {
								frame_table[i].reference_bit = 0;
							}

							// Give the current frame a second chance
							frame_table[frame_number].reference_bit = 1;
							frame_table[frame_number].last_ref_sec = sysclock->sec;
							frame_table[frame_number].last_ref_nano = sysclock->nano_sec;
						} else if (msg.action_flag == WRITE_FLAG) {
							// Update dirty bit when it modified
							fprintf(logfile, "Master: Address %d in frame %d, writing data to frame at time %d:%d\n"
										, msg.address, frame_number, sysclock->sec, sysclock->nano_sec);
							fprintf(logfile, "Master: Dirty bit of frame %d set, adding additional time to the clock\n", frame_number);
							fflush(logfile);
							incrementSysClock(sysclock, 0, 10);
							frame_table[frame_number].dirty_bit = 1;
						}
						msg.mesg_granted = TRUE;
						msgsnd(user_msgqueue_id, &msg, (sizeof(message)-sizeof(long)), 0);
					} else {
						// page fault
						// Find an empty frame
						total_page_fault++;
						fprintf(logfile, "Master: Address %d is not in a frame, pagefault\n" , msg.address);
						fflush(logfile);

						int i;
						int empty_frame_number = -1;
						for (i = 0; i < TOTAL_SYSTEM_MEMORY; i++) {
							if (allocated_frames[i] == -1) {
								empty_frame_number = i;
								break;
							}
						}

						if (empty_frame_number == -1) {
							// PAGE REPLACEMENT ALGORITHM
							// Find ref bit with 0
							int replace_frame = -1;
							int replace_frame_ref_sec  = 0;
							int replace_frame_ref_nano = 0;
							for (i = 0; i < TOTAL_SYSTEM_MEMORY; i++) {
								// See if reference bit is 0
								if (frame_table[i].reference_bit == 0){
									if (replace_frame == -1){
										replace_frame = frame_number;
										replace_frame_ref_sec = frame_table[i].last_ref_sec;
										replace_frame_ref_nano = frame_table[i].last_ref_nano;
									}
									// compare which page was allocated first
									else if (isAfter(frame_table[i].last_ref_sec,
												frame_table[i].last_ref_nano,
												replace_frame_ref_sec,
												replace_frame_ref_nano) ) {
										replace_frame = frame_number;
										replace_frame_ref_sec = frame_table[i].last_ref_sec;
										replace_frame_ref_nano = frame_table[i].last_ref_nano;

									}
								}
							}
							empty_frame_number = replace_frame;
						}
						allocated_frames[empty_frame_number] = 1;
						page_table[page_number] = empty_frame_number;
						frame_table[empty_frame_number].occupied = TRUE;
						frame_table[empty_frame_number].pid_own = msg.process_id;
						frame_table[empty_frame_number].reference_bit = 0;
						frame_table[frame_number].dirty_bit = 1;
						frame_table[empty_frame_number].last_ref_sec = sysclock->sec;
						frame_table[empty_frame_number].last_ref_nano = sysclock->nano_sec;
						msg.mesg_type = msg.process_id;
						msg.mesg_granted = TRUE;
						fprintf(logfile, "Master: Clearing frame %d and swapping in p%d page %d\n"
								, empty_frame_number, msg.process_id, page_number);
						fflush(logfile);
						msgsnd(user_msgqueue_id, &msg, (sizeof(message)-sizeof(long)), 0);
					}
				}
				// Print graph for every 1000 requests
				if (number_memory_access % 100 == 0) {
					fprintf(logfile, "Current memory layout at time %d:%d is:\n", sysclock->sec, sysclock->nano_sec);
					fprintf(logfile,"		Occupied		RefByte		DirtyBit\n");
					int i;
					for (i = 0; i< TOTAL_SYSTEM_MEMORY; i++) {
						fprintf(logfile, "Frame %d:		%s		%d		%d\n", i, frame_table[i].occupied ? "Yes" : "No", frame_table[i].reference_bit, frame_table[i].dirty_bit);
					}

					fprintf(logfile, "\n");
					fprintf(logfile, "Number of Memory access per seconds : %d\n", number_memory_access / sysclock->sec);
					fprintf(logfile, "Number of Page faults per seconds : %d\n", total_page_fault / sysclock->sec);
					fprintf(logfile, "\n");

					fflush(logfile);
				}

			}
		}
	}

	terminate();

	return EXIT_SUCCESS;
}