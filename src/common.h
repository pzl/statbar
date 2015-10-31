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

typedef struct shmem {
	pid_t server;
	char buf[BUF_SIZE];
} shmem;

void handler(int sig, siginfo_t *siginfo, void *ignore);
void catch_signals(void);
void nsleep(long nsecs);

//implemented in client and daemon
void cleanup(void);
void notified(int sig, pid_t pid, int value);

#endif
