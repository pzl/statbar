#include <sys/mman.h> //mmap()
#include <sys/stat.h> //fstat
#include <fcntl.h> //O_* defs
#include <unistd.h> //close()
#include <semaphore.h> //sem_*
#include <signal.h> //sigaction, etc
#include <stdlib.h> //exit()
#include <stdio.h>
#include "common.h"

void setup_memory(void);
void setup_semaphore(void);

static void * addr;
static sem_t * sem;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	setup_memory();
	setup_semaphore();

	catch_signals();

	while (1) {
		sem_wait(sem);
		printf("%s\n", (char *)addr);
	}

	printf("waiting for write to happen...\n");
	sem_wait(sem);

	cleanup();
	return 0;
}

void setup_memory(void) {
	int mem_fd;

	if ((mem_fd = shm_open(SHM_PATH, O_RDONLY, 0)) < 0){
		perror("accessing shared mem");
		exit(-1);
	}

	addr = mmap(NULL, BUF_SIZE, PROT_READ, MAP_SHARED, mem_fd, 0);
	if (addr == MAP_FAILED){
		perror("memmapping");
		exit(-1);
	}
	if (close(mem_fd) < 0){
		perror("closing memory FD");
		exit(-1);
	}
}

void setup_semaphore(void) {
	sem = sem_open(SEM_PATH, 0);
	if (sem == SEM_FAILED){
		perror("getting semaphore");
		exit(-1);
	}
}

void cleanup(void) {
	if (sem_close(sem) < 0) {
		perror("closing semaphore");
	}
	if (munmap(addr,BUF_SIZE) < 0){
		perror("unmapping mem");
	}
	printf("cleaned up\n");
}
