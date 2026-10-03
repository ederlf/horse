#ifndef NETNS_H
#define NETNS_H

#include <sys/types.h>

int netns_enter(const char *name);
int netns_delete(const char *name);
int netns_add(const char *name);

pid_t netns_launch(const char *name, const char *fmt, ...);

int netns_run(const char *name, const char *fmt, ...);

int netns_run_argv(const char *name, const char *cmd, int argc, char **argv);

#endif /* NETNS_H */

