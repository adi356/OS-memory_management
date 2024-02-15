CC = gcc
CFLAGS = -g -Wall

OSS_TARGET = oss
OSS_OBJ = oss.o
OSS_C = oss.c

USER_PROCESS_TARGET = user_proc
USER_PROCESS_OBJ = user_process.o
USER_PROCESS_C = user_process.c


.SUFFIXES: .c .o

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OSS_TARGET) $(USER_PROCESS_TARGET)

$(OSS_TARGET): $(OSS_OBJ)
	$(CC) -o $(OSS_TARGET) $(OSS_OBJ)

$(USER_PROCESS_TARGET): $(USER_PROCESS_OBJ)
	$(CC) -o $(USER_PROCESS_TARGET) $(USER_PROCESS_OBJ)

clean:
	rm -rf $(OSS_TARGET) $(OSS_OBJ) $(USER_PROCESS_TARGET) $(USER_PROCESS_OBJ) logfile *.o *~
