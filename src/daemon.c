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

static void setup_memory(void);
static void fetch_data(status *);
static void notify_watchers(void);
static void update_status(status *);


static shmem * mem;
static pid_t clients[MAX_CLIENTS];
static int n_clients=0;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	status stats;

	setup_memory();
	catch_signals();

	for (int i=0; i<15; i++){
		sleep(1);
		fetch_data(&stats);
		update_status(&stats);
		notify_watchers();
	}

	printf("exiting\n");
	cleanup();
	return 0;
}

static void setup_memory(void) {
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


static void fetch_data(status *stats) {
	snprintf(stats->datetime, SMALL_BUF, "10:18 AM Sat 10/31");
	snprintf(stats->network, SMALL_BUF, "192.168.1.32");
	snprintf(stats->net_tx, SMALL_BUF, "61.27 B/s 87.36 B/s");
	snprintf(stats->bluetooth, SMALL_BUF, "0");
	snprintf(stats->memory, SMALL_BUF, "14%%");
	snprintf(stats->cpu, SMALL_BUF, "3%% 5%% 17%% 1%%");
	snprintf(stats->gpu, SMALL_BUF, "28C | 10%% | 1350RPM");
	snprintf(stats->packages, SMALL_BUF, "3|2");
	snprintf(stats->runtime, SMALL_BUF, "3d");
	snprintf(stats->weather, SMALL_BUF, "40F");
	snprintf(stats->linux, SMALL_BUF, "4.2.4-1 ^");
}

void cleanup(void) {
	if (munmap(mem,sizeof(shmem)) < 0){
		perror("unmapping memory");
	}
	if (shm_unlink(SHM_PATH) < 0){
		perror("unlinking shared mem");
	}
}

static void update_status(status *stats) {
	int n_bytes;
	n_bytes = snprintf(mem->buf,BUF_SIZE, "%%{l}%s    %s    %s    %s    %s    %s    %s    %s    %s %%{r} %s    %s\n",
			stats->datetime,
			stats->network,
			stats->net_tx,
			stats->bluetooth,
			stats->memory,
			stats->cpu,
			stats->gpu,
			stats->packages,
			stats->runtime,
			stats->weather,
			stats->linux);
	if (n_bytes < 0){
		perror("snprintf, update_status");
		mem->buf[0] = '\0'; //manual super-truncation because something hit the fan
	} else if (n_bytes >= BUF_SIZE){
		fprintf(stderr, "update status was truncated. info longer than %d\n", BUF_SIZE);
	}
}

static void notify_watchers(void) {
	int i;

	for (i=0; i<n_clients; i++){
		kill(clients[i],SIGUSR1);
		//or use sigqueue to send an int
	}
}

void notified(int sig, pid_t pid, int value) {
	(void) sig;
	(void) value;
	//@todo check against MAX_CLIENTS
	//@todo prune dead clients
	clients[n_clients++] = pid;
}
