#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H 1

#include "event.h"
#include "net/topology.h"
#include "scheduler.h"

void handle_event(struct scheduler *sch, struct topology *topo, struct event_hdr **events, struct event_hdr *ev);

#endif