#include <signal.h>
#include <stdlib.h> //exit
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
}

void handler(int sig, siginfo_t *siginfo, void *ignore) {
	(void) siginfo;
	(void) ignore;

	printf("got signal %d... exiting\n", sig);
	cleanup();
	exit(-1);
}
