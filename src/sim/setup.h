#ifndef SETUP_H
#define SETUP_H 1

#include "net/topology.h"
#include "scheduler.h"
#include "conn_manager.h"

void setup(struct topology *topo, struct scheduler *sch,
           struct conn_manager *cm);

#endif