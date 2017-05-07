#ifndef APP_H
#define APP_H 1

#include "lib/netflow.h"
#include <uthash/uthash.h>

#define APP_STOP 0
#define APP_SEND 1

/* Using the types assigned by IANA makes it easier
*  to access the function that will handle a netflow/packet 
*  */
enum app_type {
    PINGV4 = 1,
    PINGV6 = 58,
};

struct app {
    uint16_t type;
    /* Start the application on time in miliseconds */    
    struct netflow (*start) (uint64_t, void*);
    /* Handle the packet  */
    int (*handle_netflow) (struct netflow*);
    UT_hash_handle hh;  
};

struct app *app_creator(uint16_t type);

#endif