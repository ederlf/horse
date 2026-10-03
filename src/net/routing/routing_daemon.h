#ifndef ROUTING_DAEMON
#define ROUTING_DAEMON 1

#define MAX_NAMESPACE_ID 32

#include<inttypes.h>

enum daemon_type {
    QUAGGA = 0,
    EXABGP = 1
};

struct routing_daemon {
    enum daemon_type type; 
    char namespace[MAX_NAMESPACE_ID];
    void (*start) (struct routing_daemon*, char*);
    void (*stop) (struct routing_daemon*);
    void (*change_config)(struct routing_daemon*, char *, uint8_t proto);
};

#endif