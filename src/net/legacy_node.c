#include "legacy_node.h"

static void free_ipv4_entry(void* entry)
{
    free((struct ipv4_route_entry*)entry);
}

static void free_ipv6_entry(void* entry)
{
    free((struct ipv6_route_entry*)entry);
}

void 
legacy_node_init(struct legacy_node *ln, uint16_t type)
{
    node_init(&ln->base, type);
    ln->arp_table = NULL;
    ln->ipv4_table = New_Patricia(32);
    ln->ipv6_table = New_Patricia(128);
}

void legacy_node_clean(struct legacy_node *ln)
{
    node_destroy_ports(&ln->base);
    Destroy_Patricia (ln->ipv4_table, free_ipv4_entry);
    Destroy_Patricia (ln->ipv6_table, free_ipv6_entry);
}