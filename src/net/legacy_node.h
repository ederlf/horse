#ifndef LEGACY_NODE_H
#define LEGACY_NODE_H 1

#include "node.h"
#include <patricia/patricia.h>
#include <uthash/uthash.h>
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

struct arp_table_entry {
    UT_hash_handle hh;          /* Make the struct hashable. */
    uint32_t ip;
    uint8_t eth_addr[ETH_LEN];
    uint32_t iface;
};

struct ipv4_route_entry {
    uint32_t next_hop;
    uint32_t netmask;
    uint32_t iface;
    uint8_t metric;
};

struct ipv6_route_entry {
    uint8_t next_hop[IPV6_LEN];
    uint8_t netmask[IPV6_LEN];
    uint32_t iface;
    uint8_t metric;
};

/* A legacy node can represent a simple host 
*  or like a legacy switch or a router   */
struct legacy_node {
    struct node base;
    struct arp_table_entry *arp_table;
    patricia_tree_t *ipv4_table;
    patricia_tree_t *ipv6_table;
};

void legacy_node_init(struct legacy_node *ln, uint16_t type);
void legacy_node_clean(struct legacy_node *ln);

#endif