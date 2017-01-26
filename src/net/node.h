#ifndef NODE_H
#define NODE_H 1

#include "port.h"
#include <uthash/uthash.h>

/* Define the possible types of nodes */
enum node_type {
    DATAPATH = 0,
    ROUTER = 1,
};

struct node {
    uint64_t uuid;      /* Sequential identification value in the simulator  */
    struct port *ports; /* Hash table of ports */
    uint16_t ports_num; /* Total number of ports */
    uint16_t type;      
    UT_hash_handle hh;  
};

void node_init(struct node* n, uint16_t type);
void node_destroy_ports(struct node *n);
void node_add_port(struct node *n, uint32_t port_id, uint8_t eth_addr[ETH_LEN]);
struct port* node_port(const struct node *n, uint32_t port_id);
#endif