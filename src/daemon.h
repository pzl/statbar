#ifndef _DAEMON_H
#define _DAEMON_H

#define MAX_CLIENTS 15
#define MAX_MODULES 15

static void read_data(status *, int fd, int i);
static void notify_watchers(void);
static void update_status(shmem *,status *);
static int launch_modules(struct pollfd[]);
static int launch_module(int i, char *dir);
static char * curdir(void);
static int spawn(char * path, const char * program);
static void onexit(void);


#endif
