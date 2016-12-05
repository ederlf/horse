#ifndef SIM_H
#define SIM_H 1

#include "scheduler.h"
#include "net/topology.h"

struct sim {
    struct topology *topo;   /* Topology of the simulation. */
    struct event_hdr *events;   /* Hash table of events.       */
    struct scheduler *sch;   /* Event scheduler.            */
};

void start(struct topology *topo);

#endif