#ifndef LEGACY_NODE_H
#define LEGACY_NODE_H 1

#include "node.h"
#include "route_table.h"
#include "arp_table.h"
#include <patricia/patricia.h>
#include <inttypes.h>

/* Administrative distance. Lowest distance wins.
*
*    Connected interface 0
*    Static route    1
*    Enhanced Interior Gateway Routing
*    Protocol (EIGRP) summary route  5
*    External Border Gateway Protocol (eBGP) 20
*    Internal EIGRP  90
*    IGRP    100
*    OSPF    110
*    Intermediate System-to-Intermediate System (IS-IS)  115
*    Routing Information Protocol (RIP)  120
*    Exterior Gateway Protocol (EGP) 140
*    On Demand Routing (ODR) 160
*    External EIGRP  170
*    Internal BGP    200
*    Unknown 255
*/


/* A legacy node can represent a simple host
*  or like a legacy switch or a router   */
struct legacy_node {
    struct node base;
    struct arp_table at;
    struct route_table rt;
};

void legacy_node_init(struct legacy_node *ln, uint16_t type);
void legacy_node_clean(struct legacy_node *ln);
void legacy_node_set_intf_ipv4(struct legacy_node *ln, uint32_t port_id,
                               uint32_t addr, uint32_t netmask);
struct netflow* find_forwarding_ports(struct legacy_node *ln,
                                      struct netflow *flow, bool ecmp);
struct netflow* l2_recv_netflow(struct legacy_node *ln, struct netflow *flow);
bool is_l3_destination(struct legacy_node *ln, struct netflow *flow);
#endif