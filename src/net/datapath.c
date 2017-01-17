/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "datapath.h"
#include "lib/util.h" 

static uint32_t current_uuid = 0;

/* Creates a new datapath. 
*
*  A datapath starts without any port assigned.  
*/
struct datapath* 
dp_new(uint64_t dp_id)
{
    struct datapath *dp = xmalloc(sizeof(struct datapath));
    int i;
    dp->uuid = current_uuid;
    dp->dp_id = dp_id;
    dp->ports_num = 0;
    /* Create flow tables*/
    for (i = 0; i < MAX_TABLES; ++i){
        dp->tables[i] = flow_table_new(i);
    }
    dp->ports = NULL;
    ++current_uuid;
    return dp;
}

void dp_destroy(struct datapath *dp)
{
    struct port *cur_port, *tmp;
    int i;
    /* Free Tables*/
    for (i = 0; i < MAX_TABLES; ++i){
         flow_table_destroy(dp->tables[i]);
    }
    /* Free ports */
    HASH_ITER(hh, dp->ports, cur_port, tmp) {
        HASH_DEL(dp->ports, cur_port);  
        free(cur_port);
    }
    free(dp);
}

void 
dp_add_port(struct datapath *dp, uint32_t port_id, uint8_t eth_addr[ETH_LEN])
{
    struct port *p = port_new(port_id, eth_addr);
    HASH_ADD(hh, dp->ports, port_id, sizeof(uint32_t), p);
    dp->ports_num++;
}

