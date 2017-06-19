#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H 1

#include "lib/sim_event.h"
#include "net/topology.h"
#include "of_manager.h"
#include "scheduler.h"

struct ev_handler {
    struct topology *topo;
    struct scheduler *sch;
    union {
        struct of_manager *om; /* Real controllers */
        //struct controller *ctrl; /* Simulated control */
    };
};

void handle_event(struct ev_handler *ev_hdl,
                     struct sim_event *ev);

#endif