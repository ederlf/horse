#ifndef QUAGGA_DAEMON
#define QUAGGA_DAEMON 1

#include "routing_daemon.h"

struct quagga_daemon;

struct quagga_daemon* quagga_daemon_new(char *namespace);
void set_quagga_daemon_zebra_file(struct quagga_daemon *d, char *fname);
void set_quagga_daemon_bgpd_file(struct quagga_daemon *d, char *fname);
void set_quagga_daemon_ospfd_file(struct quagga_daemon *d, char *fname);
void set_quagga_daemon_ospf6d_file(struct quagga_daemon *d, char *fname);
void set_quagga_daemon_ripd_file(struct quagga_daemon *d, char *fname);
void set_quagga_daemon_ripngd_file(struct quagga_daemon *d, char *fname);

#endif