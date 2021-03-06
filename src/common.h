#ifndef _COMMON_H
#define _COMMON_H

#include <sys/types.h> //pid_t
#include <signal.h>

#ifdef DEBUG
#define DEBUG_(x) do { x; } while (0)
#else
#define DEBUG_(x)
#endif

#define SMALL_BUF 4096

#define BUF_SIZE SMALL_BUF*12
#define SHM_PATH "/statbar"


#define MEM_FAILED (void *)-1

#define _FRESET "%{T-}"
#define _FTERM "%{T1}"
#define _FICON "%{T2}"

#define SENV(e,v) do { if (setenv(e,v,1) < 0) { perror("setenv"); } } while (0)


typedef struct shmem {
	pid_t server;
	char buf[BUF_SIZE];
} shmem;

typedef struct status {
	char datetime[SMALL_BUF];
	char network[SMALL_BUF];
	char net_tx[SMALL_BUF];
	char bluetooth[SMALL_BUF];
	char memory[SMALL_BUF];
	char cpu[SMALL_BUF];
	char gpu[SMALL_BUF];
	char packages[SMALL_BUF];
	char runtime[SMALL_BUF];
	char weather[SMALL_BUF];
	char linux[SMALL_BUF];
	char desktop[SMALL_BUF];
	char music[SMALL_BUF];
	char ups[SMALL_BUF];
} status;

void die(int sig, siginfo_t *siginfo, void *ignore);
void handler(int sig, siginfo_t *siginfo, void *ignore);
char *curdir(void);
void catch_signals(void);
void *setup_memory(int create);
void set_environment(void);
void set_env_coords(int x, int y);


//implemented in client and daemon
void notified(int sig, pid_t pid, int value);

#endif
