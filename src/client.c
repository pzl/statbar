//#define _GNU_SOURCE  /* for ppoll */
#include <sys/mman.h> //mmap()
#include <sys/stat.h> //fstat
#include <sys/prctl.h> //SIGHUP on parent death, prctl
#include <sys/types.h> //pid_t
#include <errno.h>
#include <fcntl.h> //O_* defs
#include <unistd.h> //close()
#include <signal.h> //sigaction, ppoll
#include <poll.h>
#include <stdlib.h> //exit()
#include <stdio.h>
#include "common.h"

#include <string.h>

static void setup_memory(void);
static void spawn_bar(int *in, int *out);
static void notify_server(void);
static void update_bar(int fd);
static void process_click(int fd);

static shmem *mem;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	int lemon_in, lemon_out;
	struct pollfd fds[1];

	setup_memory();
	spawn_bar(&lemon_in, &lemon_out);

	fds[0].fd = lemon_out;
	fds[0].events = POLLIN;
	
	notify_server();
	catch_signals();

	//@todo allow an escape for when server dies
	while (1) {
		DEBUG_(printf("waiting for wakeup signal\n"));

		//block. wait here for lemonbar click output, or update signal from server
		//we could block with read() or something, but let's plan for multiple
		//sources for the future.
		if (poll(fds, 1, -1) < 0){
			if (errno != EINTR){
				perror("poll");
				break;
			}
		}

		if (fds[0].revents & POLLIN) {
			fds[0].revents = 0; //clear for next round
			//something was clicked
			process_click(fds[0].fd);
		} else {
			//must have gotten pinged by server
			//@todo verify SIGUSR1 vs other signals (which generally just exit anyway)
			update_bar(lemon_in);
		}
	}

	signal(SIGUSR1,SIG_IGN);
	DEBUG_(printf("USR1 should be ignored now\n"));

	cleanup();
	return 0;
}

static void setup_memory(void) {
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
	DEBUG_(printf("connected to shared memory\n"));
}

static void notify_server(void) {
	kill(mem->server,SIGUSR1); //give server our PID
	DEBUG_(printf("sent ping to server (we are %d)\n",getpid()));
}

static void spawn_bar(int *lemon_in, int *lemon_out) {
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

		*lemon_in = child_in[1]; //child_in[1] is stdin to lemonbar
		*lemon_out = child_out[0]; //child_out[0] is lemonbar output
		DEBUG_(printf("waiting for lemonbar to start up\n"));
		sleep(1); //give time for lemonbar to start, pipes to be swapped
		DEBUG_(printf("spawned lemonbar\n"));
	}
}

static void update_bar(int fd) {
	int n_bytes;
	char buf[BUF_SIZE];

	if ((n_bytes = snprintf(buf, BUF_SIZE, "%s\n", mem->buf)) < 0){
		fprintf(stderr, "something went wrong copying\n");
		perror("snprintf, copying data string");
	}
	if ((n_bytes = write(fd, buf, strlen(buf))) < 0) {
		fprintf(stderr, "something went wrong writing\n");
		perror("writing to lemonbar through pipe");
	}
}

static void process_click(int fd) {
	char buf[SMALL_BUF];
	ssize_t n_bytes;

	n_bytes = read(fd, buf, SMALL_BUF);
	if (n_bytes < 0){
		perror("lemonbar click read");
	}
	buf[n_bytes-1] = 0; //ends in newline, overwrite \n with termination
	printf("got lemonbar output: %s\n", buf);
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
