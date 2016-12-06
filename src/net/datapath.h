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
*  UINT16_MAX == 2 ^ 16 datapaths 
*/
#define MAX_PORT_NUM UINT16_MAX /* 2 ^ 16 ports */

/* Definition of a switch of the network */
struct datapath {
    uint32_t uuid; /* Sequential identification value in the simulator  */
    uint64_t dp_id; /* Unique identification number of a switch in network*/ 
    struct port dp_ports[MAX_PORT_NUM]; /* Array of switch interfaces */
    uint16_t ports_num; /* Total number of ports */
    struct flow_table ft; 
    UT_hash_handle hh; /* Make the struct hashable */
};

struct datapath* dp_new(uint64_t dp_id);

#endif /*DATAPATH_H */