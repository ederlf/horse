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

struct port* port_new(uint32_t port_id, uint8_t eth_addr[ETH_LEN]) {
    struct port* p = xmalloc(sizeof(struct port));
    memset(p, 0x0, sizeof(struct port));
    p->port_id = port_id;
    memcpy(p->eth_address, eth_addr, ETH_LEN);
    p->config |= PORT_UP;
    p->state |= PORT_LIVE;
    return p;
}

