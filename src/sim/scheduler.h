#ifndef SCHEDULER_H
#define SCHEDULER_H 1

#include "event.h"
#include "lib/heap.h"
#include "net/topology.h"

struct scheduler {
    struct topology *topo;
    uint64_t clock; /* Current time of the simulation */
    struct heap* ev_queue; /* Scheduled events */  
};

struct scheduler* scheduler_create(struct topology* topo);
void scheduler_destroy(struct scheduler *sch);
void scheduler_insert(struct scheduler *sch, struct event *ev, uint64_t time);
void scheduler_dispatch(struct scheduler *sch);


#endif