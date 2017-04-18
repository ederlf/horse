#include "legacy_node.h"

void 
legacy_node_init(struct legacy_node *ln, uint16_t type)
{
    node_init(&ln->base, type);
    ln->arp_table = NULL;
    route_table_init(&ln->rt);
}

void legacy_node_clean(struct legacy_node *ln)
{
    node_destroy_ports(&ln->base);
    route_table_clean(&ln->rt);
}