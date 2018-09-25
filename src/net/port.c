/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "lib/util.h"
#include "port.h"
#include <string.h>

struct port* port_new(uint32_t port_id, uint8_t eth_addr[ETH_LEN],
                      uint32_t speed, uint32_t curr_speed) {
    struct port* p = xmalloc(sizeof(struct port));
    memset(p, 0x0, sizeof(struct port));
    p->port_id = port_id;
    memcpy(p->eth_address, eth_addr, ETH_LEN);
    p->ipv4_addr = NULL;
    p->ipv6_addr = NULL;
    p->config |= PORT_UP;
    p->state |= PORT_LIVE;
    /* TODO: Set port speed */
    p->max_speed = speed;
    p->curr_speed = curr_speed;
    buffer_state_init(&p->buffer_state);
    return p;
}

void port_add_v4addr(struct port *p, uint32_t ipv4_addr, uint32_t netmask)
{
    p->ipv4_addr = xmalloc(sizeof(struct ipv4_info));
    p->ipv4_addr->addr = ipv4_addr;
    p->ipv4_addr->netmask = netmask;
}

void port_add_v6addr(struct port *p, uint8_t ipv6_addr[IPV6_LEN],
                     uint8_t netmask[IPV6_LEN])
{
    p->ipv6_addr = xmalloc(sizeof(struct ipv6_info));
    memcpy(p->ipv6_addr, ipv6_addr, IPV6_LEN);
    memcpy(p->ipv6_addr->netmask , netmask, IPV6_LEN);
}

void 
port_set_name(struct port *p, char *name)
{
    size_t len = strlen(name);
    if ((len+1) <= MAX_PORT_NAME){
        strcpy(p->name, name);
    }
}