#include "legacy_node.h"

void 
legacy_node_init(struct legacy_node *ln, uint16_t type)
{
    node_init(&ln->base, type);
    arp_table_init(&ln->at);
    route_table_init(&ln->rt);
}

void legacy_node_clean(struct legacy_node *ln)
{
    node_destroy_ports(&ln->base);
    arp_table_clean(&ln->at);
    route_table_clean(&ln->rt);
}