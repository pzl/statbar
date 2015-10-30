#include <signal.h>
#include <stdlib.h> //exit
#include <unistd.h> //getpid
#include <time.h>
#include <stdio.h>
#include "common.h"

void catch_signals(void) {
	struct sigaction action;
	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = handler;

	sigaction(SIGINT, &action, 0);
	sigaction(SIGTERM, &action, 0);
	sigaction(SIGQUIT, &action, 0);
	sigaction(SIGABRT, &action, 0);
	sigaction(SIGSEGV, &action, 0);
	sigaction(SIGPIPE, &action, 0);
	sigaction(SIGCHLD, &action, 0);
	sigaction(SIGTSTP, &action, 0);

	sigaction(SIGUSR1, &action, 0);
}

void handler(int sig, siginfo_t *siginfo, void *ignore) {
	(void) siginfo;
	(void) ignore;

	if (sig == SIGUSR1) {
		notified(sig,siginfo->si_pid,0);
	} else {
		printf("%d: got signal %d... exiting\n", getpid(), sig);
		cleanup();
		exit(-1);
	}
}

void nsleep(long nsecs) {
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = nsecs;

	nanosleep(&delay, NULL);
}
