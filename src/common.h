#ifndef _COMMON_H
#define _COMMON_H

#include <signal.h>


#define BUF_SIZE 4096

#define SHM_PATH "/statbar"
#define SEM_PATH "/statbar"

void handler(int sig, siginfo_t *siginfo, void *ignore);
void catch_signals(void);
void cleanup(void);

#endif
