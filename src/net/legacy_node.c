#include "legacy_node.h"
#include <uthash/utlist.h>

void
legacy_node_init(struct legacy_node *ln, uint16_t type)
{
    node_init(&ln->base, type);
    arp_table_init(&ln->at);
    route_table_init(&ln->rt);
}

void legacy_node_clean(struct legacy_node *ln)
{
    node_destroy_ports(&ln->base);
    arp_table_clean(&ln->at);
    route_table_clean(&ln->rt);
}

void legacy_node_set_intf_ipv4(struct legacy_node *ln, uint32_t port_id,
                               uint32_t addr, uint32_t netmask) {
    struct port *p = node_port(&ln->base, port_id);
    port_add_v4addr(p, addr, netmask);
    /* Add entry to route table */
    struct route_entry_v4 *e = malloc(sizeof(struct route_entry_v4));
    memset(e, 0x0, sizeof(struct route_entry_v4));
    e->ip = addr & netmask;
    e->netmask = netmask;
    e->iface = port_id;
    add_ipv4_entry(&ln->rt, e);
}

/* Returns the gateway of destination IP */
uint32_t 
ip_lookup(struct legacy_node *ln, struct netflow *flow){
    struct route_entry_v4 *re = ipv4_lookup(&ln->rt, flow->match.ipv4_dst);
    if (!re){
        /* Default gateway */
        re = ipv4_lookup(&ln->rt, 0);
    }
    if (re){
        netflow_add_out_port(flow, re->iface);
        /* IP of the next hop to search in the ARP table */
        return re->gateway == 0? flow->match.ipv4_dst: re->gateway; 
    } 
    /* Could not find a route */
    return 0;   
}
            
struct netflow*
resolve_mac(struct legacy_node *ln, struct netflow *flow, uint32_t ip){
    struct arp_table_entry *ae = arp_table_lookup(&ln->at, ip);
    if (ae){
        /* Fill Missing information */
        struct out_port *op;
        LL_FOREACH(flow->out_ports, op) {
            struct port *p = node_port(&ln->base, op->port);
            flow->match.ipv4_src = p->ipv4_addr->addr;
            memcpy(flow->match.eth_src, p->eth_address, ETH_LEN);
        }
        memcpy(flow->match.eth_dst, ae->eth_addr, ETH_LEN);
    }
    else {
        /*  ARP request before the flow*/
        uint8_t bcast_eth_addr[ETH_LEN] = {0xff, 0xff, 0xff, 
                                            0xff, 0xff, 0xff};
        /* Craft request */
        struct netflow *arp_req = netflow_new();
        arp_req->match.eth_type = ETH_TYPE_ARP;
        struct out_port *op;
        LL_FOREACH(flow->out_ports, op) {
            struct port *p = node_port(&ln->base, op->port);
            /* Need to set missing ip address info */
            flow->match.ipv4_src = p->ipv4_addr->addr;
            memcpy(arp_req->match.eth_src, p->eth_address, ETH_LEN);
            memcpy(arp_req->match.arp_sha, p->eth_address, ETH_LEN);
            arp_req->match.arp_spa = p->ipv4_addr->addr;
            netflow_add_out_port(arp_req, op->port);
        }
        /* Set Destination address */
        memcpy(arp_req->match.eth_dst, bcast_eth_addr, ETH_LEN); 
        arp_req->match.arp_tpa = ip;
        arp_req->match.arp_op = ARP_REQUEST;
        arp_req->start_time = flow->start_time;
        arp_req->pkt_cnt = 1;
        arp_req->byte_cnt = 56;
        /* Put the current packet in the queue and flow becomes ARP */ 
        node_flow_push((struct node*) ln, flow);
        flow = arp_req;
    }
    return flow;
}

struct netflow* 
find_forwarding_ports(struct legacy_node *ln, struct netflow *flow)
{
    uint32_t ip = ip_lookup(ln, flow);
    /* Only forward if the next hop ip is found */
    if (ip) {
        return resolve_mac(ln, flow, ip);    
    }
    return NULL;
}