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


/* Definition of a switch of the network */
struct datapath {
    struct node base;
    uint64_t dp_id; /* Unique identification number of a switch in network*/ 
    struct flow_table *tables[MAX_TABLES]; 
};

/* Creates a new datapath. 
*
*  A datapath starts without any port assigned.  
*/
struct datapath* 
dp_new(uint64_t dp_id)
{
    struct datapath *dp = xmalloc(sizeof(struct datapath));
    int i;
    node_init(&dp->base, DATAPATH);
    dp->dp_id = dp_id;
    /* Create flow tables*/
    for (i = 0; i < MAX_TABLES; ++i){
        dp->tables[i] = flow_table_new(i);
    }
    return dp;
}

void dp_destroy(struct datapath *dp)
{
    int i;
    /* Free Tables*/
    for (i = 0; i < MAX_TABLES; ++i){
         flow_table_destroy(dp->tables[i]);
    }
    node_destroy_ports(&dp->base);
    free(dp);
}

void 
dp_add_port(struct datapath *dp, uint32_t port_id, uint8_t eth_addr[ETH_LEN])
{   
    node_add_port(&dp->base, port_id, eth_addr);
}

/* Retrieve a datapath port */
struct port* 
dp_port(struct datapath *dp, uint32_t port_id)
{
    struct port *p = node_port(&dp->base, port_id);
    return p;
}

/* The match can be modified by an action */
/* Return is a list of ports/ NULL or -1 */
void 
dp_handle_flow(struct datapath *dp, uint64_t pkt_cnt, uint64_t byte_cnt, struct flow_key *match)
{
    /* Get the input port and update rx counters*/
    uint8_t i;
    struct flow *f;
    uint32_t in_port = match->in_port;
    struct port *p = dp_port(dp, in_port);

    if (p != NULL) {
        p->stats.rx_packets = byte_cnt;
        p->stats.rx_bytes = pkt_cnt;
        /* Enter pipeline */
        for(i = 0; i < MAX_TABLES; ++i){
            f = flow_table_lookup(dp->tables[i], match);
            if (f){
            /* Execute instructions */
            }
        } 
    }
}

uint64_t 
dp_uuid(struct datapath* dp)
{
    return dp->base.uuid;
}


uint64_t 
dp_id(struct datapath* dp)
{
    return dp->dp_id;
}
