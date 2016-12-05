#ifndef SIM_H
#define SIM_H 1

#include "scheduler.h"
#include "net/topology.h"

struct sim {
    struct topology *topo;
    struct scheduler *sch;
};

void start(struct topology* topo);

#endif