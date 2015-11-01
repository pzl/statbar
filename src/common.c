#include <signal.h>
#include <stdlib.h> //exit
#include <unistd.h> //getpid
#include <time.h>
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
	cleanup();
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
		DEBUG_(printf("%d: got signal %d... exiting\n", getpid(), sig));
		//cleanup();
		//exit(-1);
	}
}

void nsleep(long nsecs) {
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = nsecs;

	nanosleep(&delay, NULL);
}
