#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H 1

#include "lib/event.h"
#include "net/topology.h"
#include "scheduler.h"

void handle_event(struct scheduler *sch, struct topology *topo,
                     struct event *ev);

#endif