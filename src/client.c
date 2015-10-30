#include <sys/mman.h> //mmap()
#include <sys/stat.h> //fstat
#include <sys/prctl.h> //SIGHUP on parent death, prctl
#include <sys/types.h> //pid_t
#include <fcntl.h> //O_* defs
#include <unistd.h> //close()
#include <signal.h> //sigaction, etc
#include <stdlib.h> //exit()
#include <stdio.h>
#include "common.h"

#include <string.h>

void setup_memory(void);
void spawn_bar(void);
void notify_server(void);

static shmem *mem;
static int input;
static int output;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	char buf[BUF_SIZE];
	int n_bytes;

	setup_memory();
	printf("connected to shared memory\n");

	spawn_bar();
	printf("spawned lemonbar\n");

	
	notify_server();
	printf("sent ping to server\n");

	catch_signals();

	while (1) {
		//@todo allow an escape for when server dies
		printf("waiting for wakeup signal\n");
		pause();
		if ((n_bytes = snprintf(buf, BUF_SIZE, "%s\n", mem->buf)) < 0){
			fprintf(stderr, "something went wrong copying\n");
			perror("snprintf, copying data string");
		}
		if ((n_bytes = write(input,buf, strlen(buf))) < 0) {
			fprintf(stderr, "something went wrong writing\n");
			perror("writing to lemonbar through pipe");
		}
	}

	signal(SIGUSR1,SIG_IGN);
	printf("USR1 should be ignored now\n");
	
	sleep(2);
	cleanup();
	return 0;
}

void setup_memory(void) {
	int mem_fd;
	void *addr;

	if ((mem_fd = shm_open(SHM_PATH, O_RDONLY, 0)) < 0){
		perror("accessing shared mem");
		exit(-1);
	}

	addr = mmap(NULL, sizeof(shmem), PROT_READ, MAP_SHARED, mem_fd, 0);
	if (addr == MAP_FAILED){
		perror("memmapping");
		exit(-1);
	}
	if (close(mem_fd) < 0){
		perror("closing memory FD");
		exit(-1);
	}

	mem = addr;
}

void notify_server(void) {
	kill(mem->server,SIGUSR1); //give server our PID
}

void spawn_bar(void) {
	int child_in[2]; //could be done one 2D array, fd[2][2], but harder to read
	int child_out[2];
	pid_t childpid;

	pipe(child_in);
	pipe(child_out);

	if ((childpid = fork()) == -1) {
		perror("fork");
		exit(-1);
	}

	if (childpid == 0) {
		//child
		if (close(child_in[1]) < 0){ //close input of first pipe
			perror("closing child input");
		}
		if (close(child_out[0]) < 0){ //close output of second pipe
			perror("closing child output");
		}

		//send SIGHUP to us when parent dies
		if (prctl(PR_SET_PDEATHSIG, SIGHUP) < 0){ //Linux only
			perror("setting up deathsig");
		}

		if (dup2(child_in[0],0) < 0){ //replace lemon stdin with pipe output
			perror("setting lemon stdin");
		}
		if (dup2(child_out[1],1) < 0){ //send lemon output through pipe2 input
			perror("setting lemon stdout");
		}

		signal(SIGUSR1,SIG_IGN);

		execlp("lemonbar", "lemonbar",
		       "-g","1920x22+0+200","-B","#383a3b","-F","#ffffff",
		       "-u","3", NULL);
		exit(0); //if lemonbar exits, don't continue running C code. DIE!
	} else {
		//parent
		if (close(child_in[0]) < 0){ //close output of child's stdin
			perror("parent closing output");
		}
		if (close(child_out[1]) <0){ //close input of its stdout
			perror("parent closing intput");
		}

		input = child_in[1]; //child_in[1] is stdin to lemonbar
		output = child_out[0]; //child_out[0] is lemonbar output
		printf("waiting for lemonbar to start up\n");
		sleep(2); //give time for lemonbar to start, pipes to be swapped
	}
}

void cleanup(void) {
	if (munmap(mem,sizeof(shmem)) < 0){
		perror("unmapping mem");
	}
	printf("cleaned up\n");
}

void notified(int sig, pid_t pid, int value) {
	(void) sig;
	(void) pid;
	(void) value;
	printf("server woke us up!\n");
}
