#ifndef DP_ACTIONS
#define DP_ACTIONS 1

#include "lib/action.h"
#include "lib/netflow.h"
#include "datapath.h"

void execute_action(struct action *act, struct netflow *flow, struct out_port **out_ports);

#endif