/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef DATAPATH_H
#define DATAPATH_H 1

#include "node.h"
#include "flow_table.h"
#include "lib/netflow.h"
#include <inttypes.h>

/* Total number of possible ports
*  UINT32_MAX == 2 ^ 32 datapaths 
*/
#define MAX_PORT_NUM UINT32_MAX /* 2 ^ 8 ports */

/* Total number of tables per datapath */
#define MAX_TABLES 128

struct datapath;

/* Use it as a list of output ports after the pipeline */
struct out_port {
    uint32_t port;
    struct out_port *next;
};

struct datapath* dp_new(uint64_t dp_id);

void dp_destroy(struct datapath *dp);

void dp_add_port(struct datapath *dp, uint32_t port_id, 
                 uint8_t eth_addr[ETH_LEN]);

struct port* dp_port(const struct datapath *dp, uint32_t port_id);

struct out_port* dp_recv_netflow(struct datapath *dp, struct netflow *flow);

void dp_send_netflow(struct datapath *dp, struct netflow *flow, 
                     uint32_t port);

void dp_handle_flow_mod(const struct datapath *dp, uint8_t table_id, struct flow *f, uint64_t time);

void clean_out_ports(struct out_port *ports);

/* Access functions*/
uint64_t dp_uuid(const struct datapath* dp);

uint64_t dp_id(const struct datapath* dp);




#endif /*DATAPATH_H */