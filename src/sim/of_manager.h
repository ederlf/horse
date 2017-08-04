#ifndef OF_MANAGER_H
#define OF_MANAGER_H 1

#include <cfluid/of_client.h>
#include "lib/sim_event.h"
#include "scheduler.h"


struct of_manager {
    struct of_client *of;
    struct scheduler *sch;
};


struct of_manager *of_manager_new(struct scheduler *sch);
void of_manager_destroy(struct of_manager *om);
void of_manager_send(struct of_manager *om, uint64_t dpid, 
                     uint8_t *buf, size_t len);
void of_manager_message_cb(struct of_conn* conn, uint8_t type, 
                             void *data, size_t len);

#endif