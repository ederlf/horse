#ifndef ROUTING_DAEMON
#define ROUTING_DAEMON 1

#define MAX_NAMESPACE_ID 32

enum daemon_type {
    QUAGGA = 0,
    EXABGP = 1
};

struct routing_daemon {
    enum daemon_type type;
    char namespace[MAX_NAMESPACE_ID];
    void (*start) (struct routing_daemon *);
    void (*stop) (struct routing_daemon*);
};

#endif