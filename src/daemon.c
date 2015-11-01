#include <sys/mman.h> //mmap and shm functions
#include <sys/types.h> //pid_t
#include <sys/prctl.h> //SIGHUP on parent death, prctl
#include <fcntl.h> //O_* defines
#include <unistd.h> //ftruncate, getpid
#include <poll.h> //poll, pollfd
#include <string.h> //strncpy, memset
#include <libgen.h> //dirname
#include <stdlib.h> //exit
#include <errno.h> //errno
#include <signal.h> //kill
#include <stdio.h>
#include "common.h"


#define MAX_CLIENTS 15
#define MAX_MODULES 15

static void setup_memory(void);
static void read_data(status *, int fd, int i);
static void notify_watchers(void);
static void update_status(status *);
static int launch_modules(struct pollfd[]);
static int spawn(char * path, const char * program);

static shmem * mem;
static pid_t clients[MAX_CLIENTS];
static int n_clients=0;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	status stats;
	struct pollfd fds[MAX_MODULES];
	int n_modules, response;

	memset(&stats,0,sizeof(stats));

	setup_memory();
	catch_signals();

	n_modules = launch_modules(fds);

	while (1) {
		DEBUG_(printf("waiting for wakeup\n"));

		if ((response = poll(fds, n_modules, -1)) < 0){
			if (errno != EINTR) {
				perror("poll");
				exit(1);
			} else {
				DEBUG_(printf("poll interrupted\n"));
				errno = 0;
				continue;
			}
		}
		DEBUG_(printf("woke up. response: %d\n", response));

		if (response > 0){
			//one or more FDs ready
			for (int i=0; i<n_modules; i++) {
				if (fds[i].revents) {
					DEBUG_(printf("due to module # %d\n", i));

					switch(fds[i].revents){
						case POLLIN:
							read_data(&stats, fds[i].fd, i);
							break;
						case POLLERR:
							fprintf(stderr,"module # %d, error occurred trying to poll\n", i);
							fds[i].fd = -1;
							break;
						case POLLHUP:
							fprintf(stderr,"module # %d, hang up on poll\n", i);
							fds[i].fd = -1;
							break;
						case POLLNVAL:
							fprintf(stderr,"module # %d, fd not open\n", i);
							fds[i].fd = -1;
							break;
					}
					fds[i].revents = 0; //clear events received
				}
			}
			update_status(&stats);
			notify_watchers();
		} else {
			fprintf(stderr, "poll exited, unknown reasons\n");
		}
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
	DEBUG_(printf("created shared memory segment\n"));
}

static int launch_modules(struct pollfd fds[]){
	char buf[SMALL_BUF]; //note that dirname() may/will modify this! copy if it will be used afterwards
	char * dir;
	ssize_t len;

	len = readlink("/proc/self/exe",buf,SMALL_BUF);
	if (len < 0){
		perror("readlink");
		exit(1);
	}
	buf[len] = 0;

	//dirname() may modify it's given param, so make a copy
	//snprintf(bufcpy, SMALL_BUF, "%s", buf);

	dir = dirname(buf);

	fds[0].fd = spawn(dir,"datetime");
	fds[1].fd = spawn(dir,"network");/*
	fds[2].fd = spawn(dir,"net_tx");
	fds[3].fd = spawn(dir,"bluetooth");
	fds[4].fd = spawn(dir,"memory");
	fds[5].fd = spawn(dir,"cpu");
	fds[6].fd = spawn(dir,"gpu");
	fds[7].fd = spawn(dir,"packages");
	fds[8].fd = spawn(dir,"runtime");
	fds[9].fd = spawn(dir,"weather");
	fds[10].fd = spawn(dir,"linux");*/

	fds[0].events = POLLIN;
	fds[1].events = POLLIN;/*
	fds[2].events = POLLIN;
	fds[3].events = POLLIN;
	fds[4].events = POLLIN;
	fds[5].events = POLLIN;
	fds[6].events = POLLIN;
	fds[7].events = POLLIN;
	fds[8].events = POLLIN;
	fds[9].events = POLLIN;
	fds[10].events = POLLIN;*/

	return 2;
}

static int spawn(char * dir, const char *module) {
	int fds[2];
	pid_t childpid;
	char path[SMALL_BUF];

	snprintf(path, SMALL_BUF, "%s/modules/%s",dir,module);

	printf("will be calling %s\n", path);

	pipe(fds);

	if ((childpid = fork()) == -1) {
		perror("fork");
		exit(-1);
	}

	if (childpid == 0) { //child
		if (close(fds[0]) < 0){ //close read end of pipe
			perror("closing child input");
		}
		if (dup2(fds[1],1) < 0){ //send script output through pipe write end
			perror("setting module stdout");
		}

		//send SIGHUP to us when parent dies
		if (prctl(PR_SET_PDEATHSIG, SIGHUP) < 0){ //Linux only
			perror("setting up deathsig");
		}
		signal(SIGUSR1,SIG_IGN);

		execlp(path, module, NULL);
		exit(1); //if script fails, die
	} else { //parent
		if (close(fds[1]) < 0){ //close write end of pipe
			perror("parent closing output");
		}

		return fds[0];
	}
}

static void read_data(status *stats, int fd, int i) {
	char * bufp;
	ssize_t n_bytes;

	switch (i) {
		case 0: bufp = stats->datetime; break;
		case 1: bufp = stats->network; break;
		case 2: bufp = stats->net_tx; break;
		case 3: bufp = stats->bluetooth; break;
		case 4: bufp = stats->memory; break;
		case 5: bufp = stats->cpu; break;
		case 6: bufp = stats->gpu; break;
		case 7: bufp = stats->packages; break;
		case 8: bufp = stats->runtime; break;
		case 9: bufp = stats->weather; break;
		case 10: bufp = stats->linux; break;
	}

	n_bytes = read(fd, bufp, SMALL_BUF);
	if (n_bytes < 0){
		perror("module data read");
	}
	if (bufp[n_bytes-1] == '\n'){
		//@todo what if output ends in multiple newlines? or has them in th middle?
		bufp[n_bytes-1] = 0;
	} else {
		bufp[n_bytes] = 0;
	}
	DEBUG_(printf("got data in: %s\n", bufp));
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
		DEBUG_(printf("notifying client %d (%d) of new input\n", i, clients[i]));
		kill(clients[i],SIGUSR1);
		//or use sigqueue to send an int
	}
}

void notified(int sig, pid_t pid, int value) {
	(void) sig;
	(void) value;
	//@todo check against MAX_CLIENTS
	//@todo prune dead clients
	DEBUG_(printf("adding client %d, client total: %d\n", pid, n_clients+1));
	clients[n_clients++] = pid;
}
