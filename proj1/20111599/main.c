#include "setting.h"

void initPipe(int ***ipc_pipe, int pipeNo)
{
	int i;

	*ipc_pipe = (int **) malloc(sizeof(int *) * pipeNo);
	for(i = 0; i < pipeNo; i++) {
		(*ipc_pipe)[i] = (int *) malloc(sizeof(int) * 2);
		if(pipe((*ipc_pipe)[i]) < 0) {
			perror("pipe init error!\n");
			exit(-1);
		}
	}
}
void dispatchIPCProcess(int **ipc_pipe)
{
	int i, pid;
	int shmid; // shared memory id
	int *type;

	// Create a shared memory for both input and output child processes
	// The allocated shared memory size : sizeof(int) which is 4
	shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT|0644);
	if(shmid == -1) {
		perror("shmget");
		exit(-1);
	}

	// Shared memory at shmid. Get an address to reference the 
	type = (int *) shmat(shmid, (int *) NULL, 0);
	*type = 1;

	// Create two child processes (input and output) via the main process
	for(i = 0; i < CHLD_PROC_NO; i++) {
		if(( pid = fork()) > 0 ) 	continue;
		else if( pid == 0 )	{
			// input process gets the type 0,
			// and output process gets the type 1.
			*type = !(*type);
			break;
		}
	}

	// Parent: main process
	if(pid)
	{
		// defined @ modeproc.c
		mainProcLoop(ipc_pipe);
	}
	// Child : input process, output process
	else 
	{
		switch(*type)
		{
		case INPUT_PROCESS:
			// defined @ modeproc.c
			inputProcLoop(ipc_pipe);
			break;
		case OUTPUT_PROCESS:
			// defined @ modeproc.c
			outputProcLoop(ipc_pipe);
			break;
		}
	}

	// Detaches the shared memory
	shmdt((int*)type);
	// Mark the segment to be destroyed
	shmctl(shmid, IPC_RMID, (struct shmid_ds *) NULL);
}
int main(void)
{
	int **ipc_pipe;

	/* 
		Initialize two unamed pipes (anonymous pipe)
		for communicating between (input and main) + (main and output)
	 */
	initPipe(&ipc_pipe, PIPE_NO);

	// Make the ipc_pipe[0][0] O_NONBLOCK
	if (fcntl(ipc_pipe[0][0], F_SETFL, O_NONBLOCK) == -1) {
		fprintf(stderr, "Call to fcntl failed.\n"); exit(1);
}
	// Make the ipc_pipe[1][0] O_NONBLOCK
	if (fcntl(ipc_pipe[1][0], F_SETFL, O_NONBLOCK) == -1) {
		fprintf(stderr, "Call to fcntl failed.\n"); exit(1);
	}

	// Dispatch main, input, output processes
	dispatchIPCProcess(ipc_pipe);

	return 0;
}
