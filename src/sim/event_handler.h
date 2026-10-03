#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H 1

#include "lib/sim_event.h"
#include "net/topology.h"
#include "net/live_flow.h"
#include "conn_manager.h"
#include "scheduler.h"

struct ev_handler {
    struct topology *topo;
    struct scheduler *sch;
    struct live_flow *live_flows; /* Hash table of running flows */
    union {
        struct conn_manager *cm; /* Real controllers */
        //struct controller *ctrl; /* Simulated control */
    };
};

void handle_event(struct ev_handler *ev_hdl,
                     struct sim_event *ev);


#endif