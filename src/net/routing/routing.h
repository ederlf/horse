#ifndef ROUTING_H
#define ROUTING_H 1

#include <arpa/inet.h>
#include <inttypes.h>
#include <uthash/uthash.h>
#include "lib/packets.h"

enum protocols {
    IGP = 9,
    EIGPR = 88,
    OSPF = 89,
    ISIS = 124,
    BGP = 179
};

struct routing {
    uint16_t type; /* Hash key */
    uint64_t asn; /*Autonomous system number, 
                   may not apply to every protocol but does not harm */
    uint32_t router_id;
    void (*start) (struct routing *, char *);                      
    void (*advertise)(struct routing *);
    void (*clean)(struct routing *, char *);
    UT_hash_handle hh; /* Make it hashable */
};

void routing_init(struct routing *r, uint16_t type);

#endif
