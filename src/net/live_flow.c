#include "live_flow.h"
#include "lib/util.h"

/* Starts from 0 */
static uint64_t flow_id = 0;

struct live_flow* live_flow_new(uint64_t node_id, struct netflow *nf)
{
	struct live_flow *f = xmalloc(sizeof(struct live_flow));
	f->flow_id = flow_id++;
	f->node_id = node_id;
	f->flow = nf;
	return f;
}

void live_flow_destroy(struct live_flow *lf)
{
	netflow_destroy(lf->flow);
	free(lf);
}