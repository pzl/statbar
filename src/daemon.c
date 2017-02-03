#include <sys/mman.h> //mmap and shm functions
#include <sys/types.h> //pid_t
#include <sys/prctl.h> //SIGHUP on parent death, prctl
#include <fcntl.h> //O_* defines
#include <unistd.h> //ftruncate, getpid
#include <poll.h> //poll, pollfd
#include <string.h> //strncpy, memset, strndup
#include <libgen.h> //dirname
#include <stdlib.h> //exit, atexit
#include <errno.h> //errno
#include <signal.h> //kill
#include <stdio.h>
#include "common.h"
#include "daemon.h"

static pid_t clients[MAX_CLIENTS];
static int n_clients=0;

int main(int argc, char const *argv[]) {
	(void) argc;
	(void) argv;

	shmem *mem;
	status stats;
	struct pollfd fds[MAX_MODULES];
	int n_modules, response;

	memset(&stats,0,sizeof(stats));

	if ((mem = setup_memory(1)) == MEM_FAILED){
		fprintf(stderr, "couldn't initialize shared memory\n");
		exit(-1);
	}
	mem->server = getpid();

	if (atexit(onexit) != 0) {
		perror("atexit");
		exit(-1);
	}

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
					DEBUG_(printf("due to module # %d, got %d\n", i,fds[i].revents));

					if (fds[i].revents & POLLIN){
						read_data(&stats, fds[i].fd, i);
					}
					if (fds[i].revents & POLLERR){
						fprintf(stderr,"module # %d, error occurred trying to poll\n", i);
						fds[i].fd = -1;
					}
					if (fds[i].revents & POLLHUP){
						fprintf(stderr,"module # %d, hang up on poll\n", i);
						fds[i].fd = -1;
					}
					if (fds[i].revents & POLLNVAL){
						fprintf(stderr,"module # %d, fd not open\n", i);
						fds[i].fd = -1;
					}
					fds[i].revents = 0; //clear events received
				}
			}
			update_status(mem,&stats);
			notify_watchers();
		} else {
			fprintf(stderr, "poll exited, unknown reasons\n");
		}
	}

	printf("exiting\n");
	return 0;
}

static int launch_modules(struct pollfd fds[]){
	int i;
	char *path = curdir();

	for (i=0; i<12; i++){
		fds[i].fd = launch_module(i,path);
		fds[i].events=POLLIN;
	}

	free(path);
	return i;
}

static int launch_module(int i, char *path){
	char * dir;
	const char * module;

	dir = ( path == NULL ) ? curdir() : path;

	switch (i){
		case 0: module = "datetime"; break;
		case 1: module = "network"; break;
		case 2: module = "transfer"; break;
		case 3: module = "bluetooth"; break;
		case 4: module = "mem"; break;
		case 5: module = "cpu"; break;
		case 6: module = "gpu"; break;
		case 7: module = "packages"; break;
		case 8: module = "runtime"; break;
		case 9: module = "weather"; break;
		case 10: module = "linux"; break;
		case 11: module = "desktop"; break;
	}
	return spawn(dir, module);
}

static int spawn(char * dir, const char *module) {
	int fds[2];
	pid_t childpid;
	char path[SMALL_BUF];

	snprintf(path, SMALL_BUF, "%s/modules/%s",dir,module);

	DEBUG_(printf("will be calling %s\n", path));

	pipe(fds);

	if ((childpid = fork()) == -1) {
		perror("fork");
		exit(-1);
	}

	if (childpid == 0) { //child
		if (close(fds[0]) < 0){ //close read end of pipe
			perror("closing child input");
		}
		if (dup2(fds[1],STDOUT_FILENO) < 0){ //send script output through pipe write end
			perror("setting module stdout");
		}

		//send SIGHUP to us when parent dies
		if (prctl(PR_SET_PDEATHSIG, SIGHUP) < 0){ //Linux only
			perror("setting up deathsig");
		}
		signal(SIGUSR1,SIG_IGN);

		set_environment();

		execlp(path, module, NULL);
		exit(1); //if script fails, die
	} else { //parent
		if (close(fds[1]) < 0){ //close write end of pipe
			perror("parent closing output");
		}

		return fds[0];
	}
}

static void read_data(status *stats, int fd, int moduleno) {
	char * bufp;
	int i;
	ssize_t n_bytes;

	switch (moduleno) {
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
		case 11: bufp = stats->desktop; break;
	}

	n_bytes = read(fd, bufp, SMALL_BUF-4);
	if (n_bytes < 0){
		perror("module data read");
	}

	//strip any newlines
	for (i=0; i<n_bytes; i++){
		if (bufp[i] == '\n') {
			bufp[i] = ' ';
		}
	}
	if (moduleno != 10){
		bufp[n_bytes] = ' '; //add space padding
		bufp[n_bytes+1] = ' ';
		bufp[n_bytes+2] = ' ';
		bufp[n_bytes+3] = ' ';
		bufp[n_bytes+4]='\0'; // force null termination
	} else {
		bufp[n_bytes] = '\0';
	}

	DEBUG_(printf("got data in: %s\n", bufp));
}


static void update_status(shmem *mem, status *stats) {
	int n_bytes;
	n_bytes = snprintf(mem->buf,BUF_SIZE, "%%{l} %s%s%s%s%s%s%s%s%s%s%%{r}%s%s\n",
			stats->datetime,
			stats->desktop,
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
	for (int i=0; i<n_clients; i++){
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

static void onexit(void) {
	if (shm_unlink(SHM_PATH) < 0){
		perror("unlinking shared mem");
	}
	printf("cleaned up\n");
}
