#ifndef PING_H
#define PING_H 1

#include "lib/netflow.h"

int ping_handle_netflow(struct netflow *flow);
struct netflow *ping_start(uint64_t start, void* ip_dst);

#endif