#ifndef EXABGP_DAEMON
#define EXABGP_DAEMON 1

#include "routing_daemon.h"

struct exabgp_daemon;

struct exabgp_daemon* exabgp_daemon_new(char *namespace);
void set_exabgp_daemon_config_file(struct exabgp_daemon *d, char *fname);
void exabgp_daemon_add_port_ip(struct exabgp_daemon *d, char *ip);
#endif