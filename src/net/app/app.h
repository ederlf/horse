#ifndef APP_H
#define APP_H 1

#include "lib/netflow.h"
#include "raw_udp.h"
#include <uthash/uthash.h>

/* Using the types assigned by IANA just: because 
*  */
enum app_type {
    PINGV4 = 1,
    UDP =  17,
    PINGV6 = 58,
};

struct exec {
    uint64_t id;         /* Creation id of the execution */
    uint16_t type;       /* Retrieve from apps of the node */
    uint64_t start_time; /* Initial time of the application */  
    void *args;          /* Argument specific to the execution */
    UT_hash_handle hh;  
};

/* This struct defines the behaviour of an application */
struct app {
    uint16_t type; /* Identification of the app */
    /* Start the application on time in microseconds 
     * Returns the first generated flow.
    */     
    struct netflow* (*start) (uint64_t, void*);
    /* Handle the packet  */
    int (*handle_netflow) (struct netflow*);
    UT_hash_handle hh;  
};

struct app *app_creator(uint16_t type);
struct exec *app_new_exec(uint64_t id, uint16_t type, uint64_t start_time, 
                          void *args, size_t arg_size);
void app_destroy(struct app *app);
void app_destroy_exec(struct exec *exec);

#endif