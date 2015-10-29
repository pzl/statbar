#include <sys/mman.h> //mmap and shm functions
#include <semaphore.h>
#include <fcntl.h> //O_* defines
#include <unistd.h> //ftruncate
#include <string.h> //strncpy
#include <time.h> //nanosleep
#include <stdlib.h> //exit
#include <errno.h> //errno
#include <stdio.h>
#include "common.h"


void setup_semaphore(void);
void setup_memory(void);
void fetch_data(void);
void notify_watchers(void);


static void * addr; //shared memory address
static sem_t * sem; //notify clients we're done writing


int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	setup_memory();
	setup_semaphore();

	catch_signals();

	fetch_data();
	notify_watchers();

	sleep(5);
	printf("exiting\n");
	cleanup();
	return 0;
}

void setup_memory(void) {
	int mem_fd;

	if ((mem_fd = shm_open(SHM_PATH, O_CREAT|O_TRUNC|O_RDWR, 0600)) < 0){
		perror("creating shared memory");
		exit(-1);
	}
	if (ftruncate(mem_fd, BUF_SIZE) < 0){
		perror("resizing shared mem");
		exit(-1);
	}

	addr = mmap(NULL, BUF_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0);
	if (addr == MAP_FAILED){
		perror("mmapping");
		exit(-1);
	}
	if (close(mem_fd) < 0){
		perror("closing memory file");
	}
}

void setup_semaphore(void) {
	sem = sem_open(SEM_PATH, O_CREAT|O_EXCL, 0600, 0);
	if (sem == SEM_FAILED){
		if (errno == EEXIST) {
			printf("cleaning up leftover semaphore\n");
			//last time we ran, sem didn't get cleaned up!
			if (sem_unlink(SEM_PATH) < 0){ //remove existing
				perror("could not clean up existing semaphore");
				exit(-1);
			}
			setup_semaphore(); //try again
			return;
		}
		perror("creating semaphore");
		exit(-1);
	}
}

void fetch_data(void) {
	sleep(2);
	printf("copying data into shared memory\n");
	strncpy(addr,"foobar",BUF_SIZE);
	printf("copied\n");
}

void cleanup(void) {
	if (munmap(addr,BUF_SIZE) < 0){
		perror("unmapping memory");
	}
	if (shm_unlink(SHM_PATH) < 0){
		perror("unlinking shared mem");
	}
	if (sem_close(sem) < 0){
		perror("closing semaphore");
	}
	if (sem_unlink(SEM_PATH) < 0){
		perror("unlinking semaphore");
	}
}

void notify_watchers(void) {
	int semval;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000; //0.01ms, 1us

	//POST the sem for as many clients as there are
	sem_getvalue(sem,&semval);
	while (semval==0){
		sem_post(sem);
		nanosleep(&delay, NULL); //if we don't wait a tic before getting the value, we won't catch the client's decrementing
		sem_getvalue(sem,&semval);
	}

}
