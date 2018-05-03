#ifndef CONN_MANAGER_H
#define CONN_MANAGER_H 1

#include <cfluid/of_client.h>
#include "lib/sim_event.h"
#include "scheduler.h"


struct conn_manager {
    struct of_client *of;
    struct scheduler *sch;
};


struct conn_manager *conn_manager_new(struct scheduler *sch);
void conn_manager_destroy(struct conn_manager *om);
void conn_manager_send_of(struct conn_manager *om, uint64_t dpid, 
                     uint8_t *buf, size_t len);

#endif