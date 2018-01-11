#ifndef ARP_TABLE_H
#define ARP_TABLE_H 1

#include "lib/packets.h"
#include <uthash/uthash.h>

enum arp_op {
    ARP_REQUEST = 1,
    ARP_REPLY = 2,
};

struct arp_table_entry {
    uint32_t ip;
    uint8_t eth_addr[ETH_LEN];
    uint32_t iface;
    UT_hash_handle hh;          /* Make the struct hashable. */
};

struct arp_table {
    struct arp_table_entry *entries;
};

void arp_table_init(struct arp_table *at);
void arp_table_clean(struct arp_table *at);
void arp_table_add_entry(struct arp_table *at, struct arp_table_entry *e);
struct arp_table_entry* arp_table_lookup(struct arp_table *at, uint32_t ip);
struct arp_table_entry* arp_table_entry_new(uint32_t ip, uint8_t *eth_addr, uint32_t iface);

#endif