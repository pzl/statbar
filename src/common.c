#include <sys/mman.h>
#include <fcntl.h> //O_* defs
#include <libgen.h> //dirname
#include <signal.h>
#include <string.h> //strndup
#include <stdlib.h> //exit, setenv
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

	SENV("C_RST","%{F-}");
	SENV("C_BG","%{F#383a3b}");
	SENV("C_TITLE","%{F#708090}");
	SENV("C_DISABLE","%{F#222222}");
	SENV("C_WARN","%{F#aa0000}");
	SENV("C_CAUTION","%{F#CA8B18}");

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


	SENV("ic_monitor", _FSIJI "\ue09f" _FRESET );

	SENV("ic_graphics", _FSIJI "\ue1f5" _FRESET);

	SENV("ic_music", _FSIJI "\ue1a6" _FRESET);
	SENV("ic_headphones", _FSIJI "\ue04d" _FRESET);
	SENV("ic_quarter_note", _FUUSHI "\u2669" _FRESET);
	SENV("ic_eighth_note", _FUUSHI "\u266a" _FRESET);
	SENV("ic_dbl_quarter_note", _FSIJI "\ue05c" _FRESET);
	SENV("ic_dbl_eighth_note", _FUUSHI "\u266c" _FRESET);
	SENV("ic_play", _FSIJI "\ue058" _FRESET);
	SENV("ic_pause", _FSIJI "\ue059" _FRESET);
	SENV("ic_playpause", _FUUSHI "\u23ef" _FRESET);
	SENV("ic_stop", _FSIJI "\ue057" _FRESET);
	SENV("ic_ffwd", _FSIJI "\ue05b" _FRESET);
	SENV("ic_rwd", _FSIJI "\ue055" _FRESET);
	SENV("ic_skip", _FSIJI "\ue05a" _FRESET);
	SENV("ic_back", _FSIJI "\ue054" _FRESET);

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
