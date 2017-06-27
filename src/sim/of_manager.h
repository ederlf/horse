#ifndef OF_MANAGER_H
#define OF_MANAGER_H 1

#include <cfluid/of_client.h>
#include "lib/sim_event.h"
#include "scheduler.h"


struct of_manager {
    struct of_client *of;
    struct scheduler *sch;
};


struct of_manager *of_manager_new(void);
void of_manager_send(struct of_manager *om, struct sim_event *ev);
void of_manager_message_cb(struct of_conn* conn, uint8_t type, 
                             void *data, size_t len);

#endif