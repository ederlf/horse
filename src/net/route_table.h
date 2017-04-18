#ifndef ROUTE_TABLE_H
#define ROUTE_TABLE_H 1

#include "lib/packets.h"
#include <patricia/patricia.h>

struct route_entry_v4 {
    uint32_t ip;
    uint32_t netmask;
    uint32_t next_hop;
    uint32_t iface;
    uint8_t metric;
};

struct route_entry_v6 {
    uint8_t ip[IPV6_LEN];
    uint8_t netmask[IPV6_LEN];
    uint8_t next_hop[IPV6_LEN];
    uint32_t iface;
    uint8_t metric;
};

struct route_table {
    patricia_tree_t *ipv4_table;
    patricia_tree_t *ipv6_table;
};

void route_table_init(struct route_table *rt);
void route_table_clean(struct route_table *rt);
void add_ipv4_entry(struct route_table *rt, struct route_entry_v4 *e);
void add_ipv6_entry(struct route_table *rt, struct route_entry_v6 *e);

#endif