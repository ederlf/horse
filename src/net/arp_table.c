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

void 
arp_table_clean(struct arp_table *at)
{
    struct arp_table_entry *cur_entry, *tmp;
    /* Clean links */
    HASH_ITER(hh, at->entries, cur_entry, tmp) {
        HASH_DEL(at->entries, cur_entry);  
        free(cur_entry);
    }
}

struct arp_table_entry* arp_table_lookup(struct arp_table *at, uint32_t ip)
{
    struct arp_table_entry *entry;
    HASH_FIND(hh, at->entries, &ip, sizeof(struct arp_table_entry), entry);
    return entry;
}