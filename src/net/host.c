#include "host.h"
#include "legacy_node.h"
#include "lib/util.h"

struct host {
    struct legacy_node ep; /* Node is an endpoint */
    /* Add stuff possibly stuff related to a node */
    void (*application)(void);
};

struct host *
host_new(void)
{
    struct host *h = xmalloc(sizeof(struct host));
    legacy_node_init(&h->ep, HOST);
    h->ep.base.recv_netflow = host_recv_netflow;
    h->ep.base.send_netflow = host_send_netflow;
    // h->application = application;
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

void
host_set_intf_ipv4(struct host *h, uint32_t port_id, uint32_t addr, uint32_t netmask){
    struct port *p = host_port(h, port_id);
    port_add_v4addr(p, addr, netmask);
}

static int 
l2_recv_netflow(struct host *h, struct netflow *flow){

    struct port *p = host_port(h, flow->match.in_port);
    uint8_t *eth_dst = flow->match.eth_dst; 
    /* Return 1 if destination  is the node */
    return !(memcmp(p->eth_address, eth_dst, ETH_LEN));
}

static int 
l3_recv_netflow(struct host *h, struct netflow *flow){

    struct port *p = host_port(h, flow->match.in_port);
    /* IPv4 Assigned and flow is IPv4 */
    if (p->ipv4_addr && flow->match.ipv4_dst){
        return p->ipv4_addr->addr == flow->match.ipv4_dst;
    }
    else if (p->ipv6_addr){
        return !(memcmp(p->ipv6_addr, flow->match.ipv6_dst, IPV6_LEN));
    }
    return 0;
}

struct out_port 
*host_recv_netflow(struct node *n, struct netflow *flow)
{
    struct host *h = (struct host*) n;
    struct out_port *ports = NULL;
    printf("Received HOST %d %p\n", h->ep.base.type, flow);
    /* Check MAC and IP addresses. Only returns true if
       both have the node destination MAC and IP       */
    if (!l2_recv_netflow(h, flow) && !l3_recv_netflow(h, flow)){
        return NULL;
    }

    /* If gets here send to application */

    return ports;
}

void host_send_netflow(struct node *n, struct netflow *flow, 
                     uint32_t port)
{
    struct host *h = (struct host*) n;
    printf("Sent HOST %d %p %d\n", h->ep.base.type, flow, port);
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