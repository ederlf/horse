#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H 1

#include "lib/sim_event.h"
#include "net/topology.h"
#include "scheduler.h"

void handle_event(struct scheduler *sch, struct topology *topo,
                     struct sim_event *ev);

#endif