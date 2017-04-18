#include "route_table.h"
#include "lib/util.h"
#include <stdlib.h>

static void free_ipv4_entry(void* entry)
{
    free((struct ipv4_route_entry*)entry);
}

static void free_ipv6_entry(void* entry)
{
    free((struct ipv6_route_entry*)entry);
}

void 
route_table_init(struct route_table *rt)
{
    rt->ipv4_table = New_Patricia(32);
    rt->ipv6_table = New_Patricia(128);
}

void 
route_table_clean(struct route_table *rt)
{
    Destroy_Patricia (rt->ipv4_table, free_ipv4_entry);
    Destroy_Patricia (rt->ipv6_table, free_ipv6_entry);
}

void 
add_ipv4_entry(struct route_table *rt, struct route_entry_v4 *e)
{
    int bitlen = 32 - ntz(e->netmask);
    prefix_t *pref = New_Prefix(AF_INET, &e->ip, bitlen);

    // patricia_node_t *node;

    // prefix = ascii2prefix (AF_INET, string);
    // printf ("make_and_lookup: %s/%d\n", prefix_toa (prefix), prefix->bitlen);
    patricia_lookup (rt->ipv4_table, pref);
    Deref_Prefix (pref);
    // return (node);
}

// void 
// add_ipv6_entry(struct route_table *rt, struct route_entry_v6 *e)
// {
//     int bitlen = 32 - ntz(e->netmask);
// }