#include "arp_table.h"

void 
arp_table_init(struct arp_table *at)
{
    at->entries = NULL;
}

void 
arp_table_add_entry(struct arp_table *at, struct arp_table_entry *e)
{
    HASH_ADD(hh, at->entries, ip, sizeof(uint32_t), e);
}

