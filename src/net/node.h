#ifndef NODE_H
#define NODE_H 1

#include "port.h"
#include "lib/netflow.h"
#include <uthash/uthash.h>

#define BUFFER_MAX 3
#define MAX_NODE_NAME 16

/* Define the possible types of nodes */
enum node_type {
    DATAPATH = 0,
    ROUTER = 1,
    L2_SWITCH = 2,
    HOST = 3,
};

/* This buffer has nothing to do with the capacity, it is just to queue
   flows that cannot be sent before the the completion of an event. e.g: after an ARP REPLY.
*/
struct buffer {
    struct netflow *flows[BUFFER_MAX]; /* Store the flow id */
    int tail;  
};



struct node {
    uint64_t uuid;      /* Sequential identification value 
                           in the simulator  */
    char name[16];      /* Identification for the python binding */
    struct port *ports; /* Hash table of ports */
    uint16_t ports_num; /* Total number of ports */
    uint16_t type;
    struct buffer flow_buff; /* Buffer for packets waiting to be sent */
    struct buffer_state buffer_state; 
    struct netflow* (*recv_netflow) (struct node *, struct netflow *); /* true indicates 
                                                         further processing */
    void (*send_netflow) (struct node *, struct netflow *, uint32_t out_port);
    UT_hash_handle hh;  
};

void node_init(struct node* n, uint16_t type);
void node_destroy_ports(struct node *n);
void node_add_port(struct node *n, uint32_t port_id, uint8_t eth_addr[ETH_LEN],
                   uint32_t speed, uint32_t curr_speed);
struct port* node_port(const struct node *n, uint32_t port_id);
void node_add_tx_time(struct node *n, uint32_t out_port, 
                      struct netflow *flow);
void node_update_port_stats(struct node *n, struct netflow *flow,
                            uint32_t out_port);
bool node_is_buffer_empty(struct node *n);
bool node_flow_push(struct node *n, struct netflow *flow);
struct netflow *node_flow_pop(struct node *n);
void node_calculate_loss(struct node *n, struct netflow *nf, uint32_t out_port);
void node_update_port_capacity(struct node *n, int bits, uint32_t out_port);
int node_calculate_port_loss(struct node *n, struct netflow *nf,
                             uint32_t out_port);

#endif