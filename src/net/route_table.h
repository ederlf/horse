#ifndef ROUTE_TABLE_H
#define ROUTE_TABLE_H 1

#include "lib/packets.h"
#include <patricia/patricia.h>

enum route_entry_flags {
    ROUTE_UP = 0,
    ROUTE_GW = 1,
    ROUTE_HOST = 2,
};

struct route_entry_v4 {
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t iface;
    uint8_t flags;
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
struct route_entry_v4 *ipv4_lookup(const struct route_table *rt, uint32_t ip,
                                   uint32_t hash);
struct route_entry_v4 *ipv4_search_exact(const struct route_table *rt,
                                         uint32_t ip, uint8_t cidr);
void add_ipv6_entry(struct route_table *rt, struct route_entry_v6 *e);
struct route_entry_v6 *ipv6_lookup(const struct route_table *rt,
                                   uint8_t ip[IPV6_LEN]);

#endif