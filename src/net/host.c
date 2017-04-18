#include "host.h"
#include "legacy_node.h"
#include "lib/util.h"

struct host {
    struct legacy_node ep; /* Node is an endpoint */
    /* Add stuff possibly stuff related to a node */
    void (*application)(void);
};

struct host *
host_new(void (*application)(void))
{
    struct host *h = xmalloc(sizeof(struct host));
    legacy_node_init(&h->ep, HOST);
    h->application = application;
    return h;
}

void 
host_destroy(struct host *h)
{
    legacy_node_clean(&h->ep);
    free(h);
}

void 
host_add_port(struct host *h, uint32_t port_id, uint8_t eth_addr[ETH_LEN], uint32_t speed, uint32_t curr_speed)
{   
    node_add_port( (struct node*) h, port_id, eth_addr, speed, curr_speed);
}

/* Retrieve a datapath port */
struct port* 
host_port(const struct host *h, uint32_t port_id)
{
    struct port *p = node_port( (struct node*) h, port_id);
    return p;
}

uint64_t 
host_uuid(const struct host* h)
{
    return h->ep.base.uuid;
}