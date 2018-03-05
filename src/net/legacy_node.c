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

void legacy_node_set_intf_ipv4(struct legacy_node *ln, uint32_t port_id,
                               uint32_t addr, uint32_t netmask) {
    struct port *p = node_port(&ln->base, port_id);
    port_add_v4addr(p, addr, netmask);
    /* Add entry to route table */
    struct route_entry_v4 *e = malloc(sizeof(struct route_entry_v4));
    memset(e, 0x0, sizeof(struct route_entry_v4));
    e->ip = addr & netmask;
    e->netmask = netmask;
    e->iface = port_id;
    add_ipv4_entry(&ln->rt, e);
}
            

