#ifndef ARP_TABLE_H
#define ARP_TABLE_H 1

#include "lib/packets.h"
#include <uthash/uthash.h>

struct arp_table_entry {
    UT_hash_handle hh;          /* Make the struct hashable. */
    uint32_t ip;
    uint8_t eth_addr[ETH_LEN];
    uint32_t iface;
};

struct arp_table {
    struct arp_table_entry *entries;
};

void arp_table_init(struct arp_table *at);
void arp_table_add_entry(struct arp_table *at, struct arp_table_entry *e);

#endif