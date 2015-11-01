#ifndef _COMMON_H
#define _COMMON_H

#include <sys/types.h> //pid_t
#include <signal.h>

#ifdef DEBUG
#define DEBUG_(x) do { x; } while (0)
#else
#define DEBUG_(x)
#endif

#define BUF_SIZE 4096
#define SHM_PATH "/statbar"

#define SMALL_BUF 1024

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
} status;

void handler(int sig, siginfo_t *siginfo, void *ignore);
void catch_signals(void);
void nsleep(long nsecs);

//implemented in client and daemon
void cleanup(void);
void notified(int sig, pid_t pid, int value);

#endif
