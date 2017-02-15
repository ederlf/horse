#ifndef DP_ACTIONS
#define DP_ACTIONS 1

#include "lib/action.h"
#include "lib/netflow.h"

struct out_port {
    uint32_t port;
    struct out_port *next;
};

void execute_action(struct action *act, struct netflow *flow, struct out_port **out_ports);

#endif