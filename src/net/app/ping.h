#ifndef PING_H
#define PING_H 1

#include "lib/netflow.h"

enum ping_types {
    ECHO_REPLY = 0,
    DST_UNREACHABLE = 3,
    ECHO = 8,
};

int ping_handle_netflow(struct netflow *flow);
struct netflow ping_start(uint64_t start, void* ip_dst);

#endif