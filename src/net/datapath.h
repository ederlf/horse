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

#include "port.h"
#include "flow_table.h"
#include <inttypes.h>
#include <uthash/uthash.h>

/* Total number of possible ports
*  UINT32_MAX == 2 ^ 32 datapaths 
*/
#define MAX_PORT_NUM UINT32_MAX /* 2 ^ 8 ports */

/* Total number of tables per datapath */
#define MAX_TABLES 128

/* Definition of a switch of the network */
struct datapath {
    uint32_t uuid; /* Sequential identification value in the simulator  */
    uint64_t dp_id; /* Unique identification number of a switch in network*/ 
    struct port *ports; /* Hash table of ports */
    uint16_t ports_num; /* Total number of ports */
    struct flow_table *tables[MAX_TABLES]; 
    UT_hash_handle hh; /* Make the struct hashable */
};

struct datapath* dp_new(uint64_t dp_id);
void dp_destroy(struct datapath *dp);
void dp_add_port(struct datapath *dp, uint32_t port_id, uint8_t eth_addr[ETH_LEN]);

#endif /*DATAPATH_H */