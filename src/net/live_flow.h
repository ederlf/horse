#ifndef LIVE_FLOW_H
#define LIVE_FLOW_H 1

#include "lib/netflow.h"
#include <uthash/uthash.h>

struct live_flow {
	uint64_t flow_id;	  /* Flow identification */
	uint64_t node_id;	  /* Where that flow is. It changes  */
	struct netflow *flow; /* flow information, it can be modified */
	UT_hash_handle hh;    /* Make it hashable */
};


struct live_flow* live_flow_new(uint64_t node_id, struct netflow *nf);
void live_flow_destroy(struct live_flow *lf);


#endif