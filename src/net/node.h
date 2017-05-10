#ifndef NODE_H
#define NODE_H 1

#include "port.h"
#include "lib/netflow.h"
#include <uthash/uthash.h>

#define BUFFER_MAX 2

/* Define the possible types of nodes */
enum node_type {
    DATAPATH = 0,
    ROUTER = 1,
    L2_SWITCH = 2,
    HOST = 3,
};

struct buffer {
    struct netflow flows[2];
    int tail;  
};

struct node {
    uint64_t uuid;      /* Sequential identification value in the simulator  */
    struct port *ports; /* Hash table of ports */
    uint16_t ports_num; /* Total number of ports */
    uint16_t type;
    struct buffer flow_buff; /* A queue for postponed flows */ 
    void (*handle_netflow) (struct node *, struct netflow *);
    UT_hash_handle hh;  
};

void node_init(struct node* n, uint16_t type);
void node_destroy_ports(struct node *n);
void node_add_port(struct node *n, uint32_t port_id, uint8_t eth_addr[ETH_LEN], uint32_t speed, uint32_t curr_speed);
struct port* node_port(const struct node *n, uint32_t port_id);
bool node_is_buffer_empty(struct node *n);
bool node_flow_push(struct node*n, struct netflow flow);
struct netflow node_flow_pop(struct node *n);
#endif