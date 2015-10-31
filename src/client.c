#include <sys/mman.h> //mmap()
#include <sys/stat.h> //fstat
#include <sys/prctl.h> //SIGHUP on parent death, prctl
#include <sys/types.h> //pid_t
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
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
void update_bar(void);

static shmem *mem;
static int input;
static int output;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	fd_set fd_in;

	setup_memory();
	DEBUG_(printf("connected to shared memory\n"));

	spawn_bar();
	DEBUG_(printf("spawned lemonbar\n"));

	
	notify_server();
	DEBUG_(printf("sent ping to server\n"));

	catch_signals();

	while (1) {
		DEBUG_(printf("waiting for wakeup signal\n"));

		FD_ZERO(&fd_in);
		FD_SET(output, &fd_in);

		//block. wait here for lemonbar click output, or update signal from server
		//we could block with read() or something, but let's plan for multiple
		//sources for the future.
		if (pselect(output+1, &fd_in, NULL, NULL, NULL, NULL) < 0){
			if (errno != EINTR){
				perror("select");
				break;
			}
		}

		if (FD_ISSET(output, &fd_in)) {
			//something was clicked
		} else {
			//must have gotten pinged by server
			update_bar();
		}

		//@todo allow an escape for when server dies




	}

	signal(SIGUSR1,SIG_IGN);
	DEBUG_(printf("USR1 should be ignored now\n"));

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

		//if pgrep -x compton; then #ee383a3b else #383a3b fi

		execlp("lemonbar", "lemonbar",
		       "-g","1920x22+0+200","-B","#ee383a3b","-F","#ffffff",
		       "-u","3",
		       "-f","-*-terminus-medium-*-*-*-12-*-*-*-*-*-iso10646-*",
		       "-f","-*-lemon-medium-*-*-*-10-*-75-75-*-*-iso10646-*",
		       "-f","-*-uushi-*-*-*-*-*-*-75-75-*-*-iso10646-*",
		       "-f","-*-siji-*-*-*-*-10-*-75-75-*-*-iso10646-*",
		       NULL);
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
		sleep(1); //give time for lemonbar to start, pipes to be swapped
	}
}

void update_bar(void) {
	int n_bytes;
	char buf[BUF_SIZE];

	if ((n_bytes = snprintf(buf, BUF_SIZE, "%s\n", mem->buf)) < 0){
		fprintf(stderr, "something went wrong copying\n");
		perror("snprintf, copying data string");
	}
	if ((n_bytes = write(input,buf, strlen(buf))) < 0) {
		fprintf(stderr, "something went wrong writing\n");
		perror("writing to lemonbar through pipe");
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
	DEBUG_(printf("server woke us up!\n"));
}
