#include "node.h"
#include <stdio.h>

static uint64_t current_uuid = 0;

void 
node_init(struct node* n, uint16_t type)
{
    n->uuid = current_uuid;
    n->ports_num = 0;
    n->ports = NULL;
    n->type = type;
    ++current_uuid;
}

void 
node_destroy_ports(struct node *n)
{
    /* Free ports */
    struct port *cur_port, *tmp;
    HASH_ITER(hh, n->ports, cur_port, tmp) {
        HASH_DEL(n->ports, cur_port);  
        free(cur_port);
    }
}

void 
node_add_port(struct node *n, uint32_t port_id, uint8_t eth_addr[ETH_LEN])
{
    struct port *p = port_new(port_id, eth_addr);
    HASH_ADD(hh, n->ports, port_id, sizeof(uint32_t), p);
    n->ports_num++;
}

/* Retrieve a datapath port */
struct port* 
node_port(struct node *n, uint32_t port)
{
    struct port *p;
    HASH_FIND(hh, n->ports, &port, sizeof(uint32_t), p);
    return p;
}