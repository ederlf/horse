#include "route_table.h"
#include "lib/util.h"
#include <stdlib.h>
#include <uthash/utarray.h>
#include <log/log.h>


UT_icd route_entry_icd = {sizeof(struct route_entry_v4), NULL, NULL, NULL};

static void 
free_ipv4_entry(void* entry)
{
    utarray_free((UT_array*) entry);
    // free((struct route_entry_v4*)entry);
}

static void 
free_ipv6_entry(void* entry)
{
    utarray_free((UT_array*)entry);
    // free((struct route_entry_v6*)entry);
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
    UT_array *next_hops;
    patricia_node_t *node;
    uint32_t ip = htonl(e->ip);
    int bitlen = 32 - ntz(e->netmask);
    prefix_t *pref = New_Prefix(AF_INET, &ip, bitlen);
    node = patricia_lookup (rt->ipv4_table, pref);
    Deref_Prefix (pref);
    if (node->data == NULL){
        utarray_new(next_hops, &route_entry_icd);
        node->data = (void*) next_hops;
    }
    else {
        next_hops = (UT_array*) node->data;
    }
    utarray_push_back(next_hops, e);
}

struct route_entry_v4*
ipv4_search_exact(const struct route_table *rt,
                    uint32_t ip, uint8_t cidr)
{
    prefix_t *pref = New_Prefix(AF_INET, &ip, cidr);
    patricia_node_t *node = patricia_search_exact(rt->ipv4_table, pref);
    Deref_Prefix (pref);
    if (node != NULL){
        UT_array *next_hops = node->data;
        return (struct route_entry_v4*) utarray_eltptr(next_hops, 0);
    }
    return NULL;
}

struct route_entry_v4* 
ipv4_lookup(const struct route_table *rt, uint32_t ip, uint32_t hash)
{
    ip = htonl(ip);
    prefix_t *pref = New_Prefix(AF_INET, &ip, 32);
    patricia_node_t *node = patricia_search_best(rt->ipv4_table, pref);
    Deref_Prefix (pref);
    if (node != NULL){
        UT_array *next_hops = node->data;
        size_t n = utarray_len(next_hops);
        unsigned index = hash % n;
        log_debug("Route chosen is %u From total %ld with hash %u", index, n, hash);
        return (struct route_entry_v4*) utarray_eltptr(next_hops, index);
    }
    return NULL;
}

// void 
// add_ipv6_entry(struct route_table *rt, struct route_entry_v6 *e)
// {
//     int bitlen = 32 - ntz(e->netmask);
// }