#ifndef NODE_H
#define NODE_H 1

#include "port.h"
#include "lib/netflow.h"
#include <uthash/uthash.h>

/* Define the possible types of nodes */
enum node_type {
    DATAPATH = 0,
    ROUTER = 1,
    L2_SWITCH = 2,
    HOST = 3,
};

/* List of possible output ports after node processing */
struct out_port {
    uint32_t port;
    struct out_port *next;
};

struct node {
    uint64_t uuid;      /* Sequential identification value in the simulator  */
    struct port *ports; /* Hash table of ports */
    uint16_t ports_num; /* Total number of ports */
    uint16_t type;
    struct out_port* (*recv_netflow) (struct node *n, struct netflow *f);
    void  (*send_netflow) (struct node *n, struct netflow *f, uint32_t port);
    UT_hash_handle hh;  
};

void node_init(struct node* n, uint16_t type);
void node_destroy_ports(struct node *n);
void node_add_port(struct node *n, uint32_t port_id, uint8_t eth_addr[ETH_LEN], uint32_t speed, uint32_t curr_speed);
struct port* node_port(const struct node *n, uint32_t port_id);
#endif