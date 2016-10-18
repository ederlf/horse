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
#include <inttypes.h>
#include <uthash/uthash.h>

#define MAX_PORT_NUM 65536 /* 2 ^ 16 ports */

struct datapath {
    uint64_t dp_id; /*Unique identification number of a switch */ 
    struct port dp_ports[MAX_PORT_NUM]; /* Array of switch interfaces */
    uint16_t ports_num; /* Total number of ports */
    /*struct flow_table Flow table */
    UT_hash_handle hh; /* Make the struct hashable */
};

struct datapath* dp_new(uint64_t dp_id);

#endif