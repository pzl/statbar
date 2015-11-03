#include <sys/mman.h>
#include <fcntl.h> //O_* defs
#include <signal.h>
#include <stdlib.h> //exit
#include <unistd.h> //getpid
#include <stdio.h>
#include "common.h"

void catch_signals(void) {
	struct sigaction action, done;
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	action.sa_sigaction = handler;

	done.sa_flags = SA_SIGINFO;
	done.sa_sigaction = die;

	sigaction(SIGINT, &done, 0);
	sigaction(SIGTERM, &done, 0);
	sigaction(SIGQUIT, &done, 0);
	sigaction(SIGABRT, &done, 0);
	sigaction(SIGSEGV, &done, 0);
	//sigaction(SIGTSTP, &done, 0);

	sigaction(SIGPIPE, &action, 0);
	sigaction(SIGCHLD, &action, 0);

	sigaction(SIGUSR1, &action, 0);
}

void die(int sig, siginfo_t *siginfo, void *ignore) {
	(void) sig;
	(void) siginfo;
	(void) ignore;

	DEBUG_(printf("Received signal %d, exiting\n", sig));
	exit(1);
}

void handler(int sig, siginfo_t *siginfo, void *ignore) {
	(void) siginfo;
	(void) ignore;

	DEBUG_(printf("SIGNAL: %d from %d\n", sig, siginfo->si_pid));

	if (sig == SIGUSR1) {
		notified(sig,siginfo->si_pid,0);
	} else if (sig == SIGCHLD){
		DEBUG_(printf("%d got SIGCHLD from %d\n", getpid(), siginfo->si_pid));
	} else {
		DEBUG_(printf("%d: got signal %d, nothing to do about it\n", getpid(), sig));
	}
}

void * setup_memory(int create) {
	int mem_fd;
	void * addr;

	mem_fd = create ? shm_open(SHM_PATH, O_CREAT|O_TRUNC|O_RDWR, 0600) : shm_open(SHM_PATH, O_RDONLY, 0);
	if (mem_fd < 0){
		perror("creating or accessing shared memory");
		return MEM_FAILED;
	}
	if (create && ftruncate(mem_fd, sizeof(shmem)) < 0){
		perror("resizing shared mem");
		return MEM_FAILED;
	}

	addr = create ? mmap(NULL, sizeof(shmem), PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0) : mmap(NULL, sizeof(shmem), PROT_READ, MAP_SHARED, mem_fd, 0);
	if (addr == MAP_FAILED){
		perror("mmapping");
		return MEM_FAILED;
	}
	if (close(mem_fd) < 0){
		perror("closing memory file");
	}

	DEBUG_(printf("created/connected to shared memory segment\n"));
	return addr;
}
