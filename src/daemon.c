#include <sys/mman.h> //mmap and shm functions
#include <sys/types.h> //pid_t
#include <sys/prctl.h> //SIGHUP on parent death, prctl
#include <fcntl.h> //O_* defines
#include <unistd.h> //ftruncate, getpid
#include <poll.h> //poll, pollfd
#include <string.h> //strncpy, memset
#include <libgen.h> //dirname
#include <stdlib.h> //exit, setenv, atexit
#include <errno.h> //errno
#include <signal.h> //kill
#include <stdio.h>
#include "common.h"


#define MAX_CLIENTS 15
#define MAX_MODULES 15

#define _FRESET "%{T-}"
#define _FTERM "%{T1}"
#define _FLEMON "%{T2}"
#define _FUUSHI "%{T3}"
#define _FSIJI "%{T4}"

#define SENV(e,v) do { if (setenv(e,v,1) < 0) { perror("setenv"); } } while (0)


static void read_data(status *, int fd, int i);
static void notify_watchers(void);
static void update_status(shmem *,status *);
static int launch_modules(struct pollfd[]);
static int spawn(char * path, const char * program);
static void set_environment(void);
static void onexit(void);

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
						default:
							fprintf(stderr, "for unknown reasons: %d\n", fds[i].revents);
							fds[i].fd = -1;
							break;
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
	fds[1].fd = spawn(dir,"network");
	fds[2].fd = spawn(dir,"transfer");
	fds[3].fd = spawn(dir,"bluetooth");
	fds[4].fd = spawn(dir,"mem");
	fds[5].fd = spawn(dir,"cpu");
	fds[6].fd = spawn(dir,"gpu");
	fds[7].fd = spawn(dir,"packages");
	fds[8].fd = spawn(dir,"runtime");
	fds[9].fd = spawn(dir,"weather");
	fds[10].fd = spawn(dir,"linux");

	fds[0].events = POLLIN;
	fds[1].events = POLLIN;
	fds[2].events = POLLIN;
	fds[3].events = POLLIN;
	fds[4].events = POLLIN;
	fds[5].events = POLLIN;
	fds[6].events = POLLIN;
	fds[7].events = POLLIN;
	fds[8].events = POLLIN;
	fds[9].events = POLLIN;
	fds[10].events = POLLIN;

	return 11;
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


static void update_status(shmem *mem, status *stats) {
	int n_bytes;
	n_bytes = snprintf(mem->buf,BUF_SIZE, "%%{l} %s    %s    %s    %s    %s    %s    %s    %s    %s %%{r} %s    %s\n",
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

static void set_environment(void) {

	if (setenv("C_RST","%{F-}",1) < 0){
		perror("setenv");
	}

	SENV("C_RST","%{F-}");
	SENV("C_BG","%{F#383a3b}");
	SENV("C_TITLE","%{F#708090}");
	SENV("C_DISABLE","%{F#222222}");
	SENV("C_WARN","%{F#aa0000}");
	SENV("C_CAUTION","%{F#CA8B18}");
//export readonly C_CPU=("%{F#CD5BBD}" "%{F#63C652}" "%{F#7684D0}" "%{F#B8B02C}")
//cannot export arrays

	SENV("F_RESET",_FRESET);
	SENV("F_TERM",_FTERM);
	SENV("F_LEMON",_FLEMON);
	SENV("F_UUSHI",_FUUSHI);
	SENV("F_SIJI",_FSIJI);

	SENV("ic_lock","\ue0a2");
	//lemon & uushi lock: 2b46 (lemon also 2baa)
	SENV("ic_fail","\u2717");
	SENV("ic_discon","\u2300");
	SENV("ic_warn",_FSIJI "\ue077" _FRESET );

	SENV("ic_arch",_FSIJI "\ue00e" _FRESET );

	SENV("ic_pacman",_FSIJI "\ue00f" _FRESET );
	SENV("ic_pacman_small",_FSIJI "\ue14d" _FRESET );
	SENV("ic_pacman_tiny",_FSIJI "\ue0a0" _FRESET );
	SENV("ic_pacman_puny",_FLEMON "\u2ba2" _FRESET );

	SENV("ic_clock", _FSIJI "\ue015" _FRESET);
	SENV("ic_clock_small",_FSIJI "\ue0a3" _FRESET );
	SENV("ic_clock_tiny",_FSIJI "\ue0a2" _FRESET );

	SENV("ic_chip",_FSIJI "\ue021" _FRESET );
	SENV("ic_chip_small",_FSIJI "\ue020" _FRESET );
	SENV("ic_chip_small_inv",_FSIJI "\ue0c5" _FRESET );
	SENV("ic_chip_tiny",_FSIJI "\ue028" _FRESET );
	SENV("ic_chip_tiny_inv",_FSIJI "\ue0c4" _FRESET );
	SENV("ic_chip_puny",_FUUSHI "\u2b66" _FRESET );
	SENV("ic_chip_micro",_FLEMON "\u2ba1" _FRESET );
	SENV("ic_chip_micro_vert",_FLEMON "\u2bbd" _FRESET );

	SENV("ic_network",_FSIJI "\ue0f3" _FRESET );
	SENV("ic_blth",_FSIJI "\ue00b" _FRESET );
	SENV("ic_blth_small",_FSIJI "\ue1b5" _FRESET );
	SENV("ic_blth_tiny",_FSIJI "\ue0b0" _FRESET );

	SENV("ic_cpu",_FSIJI "\ue026" _FRESET );

	SENV("ic_transfer",_FSIJI "\ue13f" _FRESET );
	SENV("ic_transfer_vert",_FSIJI "\ue10f" _FRESET );
}

static void onexit(void) {
	if (shm_unlink(SHM_PATH) < 0){
		perror("unlinking shared mem");
	}
	printf("cleaned up\n");
}
