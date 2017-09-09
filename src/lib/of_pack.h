#ifndef OF_PACK_H
#define OF_PACK_H 1

#include "flow.h"
#include "netflow.h"
#include "sim_event.h"
#include <loci/loci.h>

#define MAX_PACKET_IN_DATA 256

of_object_t *pack_packet_in(struct netflow *f, size_t *len);
of_object_t *pack_flow_stats_reply(struct flow **flows, uint32_t xid,
                          size_t flow_count);
#endif