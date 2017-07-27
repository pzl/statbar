#include <sys/mman.h>
#include <fcntl.h> //O_* defs
#include <libgen.h> //dirname
#include <signal.h>
#include <string.h> //strndup
#include <stdlib.h> //exit, setenv
#include <unistd.h> //getpid
#include <stdio.h>
#include "icons-in-terminal.h"
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

/*
 * returned pointer should be freed when done!
 */
char *curdir(void){
	char buf[SMALL_BUF];
	char *dir;
	ssize_t len;

	len = readlink("/proc/self/exe",buf,SMALL_BUF);
	if (len < 0){
		perror("readlink");
		exit(1);
	}
	buf[len] = 0;

	dir = dirname(buf);
	return strndup(dir,SMALL_BUF);
}

void set_environment(void) {
	const char **icon;


	SENV("C_RST","%{F-}");
	SENV("C_BG","%{F#383a3b}");
	SENV("C_TITLE","%{F#708090}");
	SENV("C_DISABLE","%{F#222222}");
	SENV("C_WARN","%{F#aa0000}");
	SENV("C_CAUTION","%{F#CA8B18}");

	SENV("F_RESET",_FRESET);
	SENV("F_TERM",_FTERM);
	SENV("F_ICON",_FICON);

	for (icon = envs; *icon; icon+=2){
		SENV(*icon, *(icon+1));
	}
}

void set_env_coords(int x, int y){
	char mousex[200];
	char mousey[200];

	if (snprintf(mousex, 200, "%d", x) < 0){
		perror("snprintf");
		return;
	}
	if (snprintf(mousey, 200, "%d", y) < 0){
		perror("snprintf");
		return;
	}

	SENV("MOUSE_X",mousex);
	SENV("MOUSE_Y",mousey);
}
