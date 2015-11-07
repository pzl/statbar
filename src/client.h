#ifndef _STAT_CLIENT_H
#define _STAT_CLIENT_H

#define DEFAULT_GEOM "1920x22+0+0"

static void spawn_bar(int *in, int *out, const char *geometry);
static int validate_geometry(const char *);
static void notify_server(pid_t server);
static void update_bar(shmem *mem, int fd);
static void process_click(int fd);


#endif
