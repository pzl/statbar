#include <sys/mman.h> //mmap and shm functions
#include <sys/types.h> //pid_t
#include <fcntl.h> //O_* defines
#include <unistd.h> //ftruncate, getpid
#include <string.h> //strncpy
#include <stdlib.h> //exit
#include <errno.h> //errno
#include <signal.h> //kill
#include <stdio.h>
#include "common.h"


#define MAX_CLIENTS 15

void setup_semaphore(void);
void setup_memory(void);
void fetch_data(int);
void notify_watchers(void);


static shmem * mem;
static pid_t clients[MAX_CLIENTS];
static int n_clients=0;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	setup_memory();

	signal(SIGUSR1,SIG_IGN);

	catch_signals();

	for (int i=0; i<15; i++){
		sleep(1);
		fetch_data(i);
		notify_watchers();
	}

	printf("exiting\n");
	cleanup();
	return 0;
}

void setup_memory(void) {
	int mem_fd;
	void * addr;

	if ((mem_fd = shm_open(SHM_PATH, O_CREAT|O_TRUNC|O_RDWR, 0600)) < 0){
		perror("creating shared memory");
		exit(-1);
	}
	if (ftruncate(mem_fd, sizeof(shmem)) < 0){
		perror("resizing shared mem");
		exit(-1);
	}

	addr = mmap(NULL, sizeof(shmem), PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0);
	if (addr == MAP_FAILED){
		perror("mmapping");
		exit(-1);
	}
	if (close(mem_fd) < 0){
		perror("closing memory file");
	}

	mem = addr;
	mem->server = getpid();
}


void fetch_data(int i) {
	char buf[BUF_SIZE];
	snprintf(buf, BUF_SIZE, "curval: %%{A:hello world:}%d%%{A}", i);
	strncpy(mem->buf,buf,BUF_SIZE);
}

void cleanup(void) {
	if (munmap(mem,sizeof(shmem)) < 0){
		perror("unmapping memory");
	}
	if (shm_unlink(SHM_PATH) < 0){
		perror("unlinking shared mem");
	}
}

void notify_watchers(void) {
	int i;

	for (i=0; i<n_clients; i++){
		kill(clients[i],SIGUSR1);
	}
}

void notified(int sig, pid_t pid, int value) {
	(void) sig;
	(void) value;
	//@todo check against MAX_CLIENTS
	//@todo prune dead clients
	clients[n_clients++] = pid;
}
