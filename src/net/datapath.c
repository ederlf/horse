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
dp_port(const struct datapath *dp, uint32_t port_id)
{
    struct port *p = node_port(&dp->base, port_id);
    return p;
}

// INSTRUCTION_APPLY_ACTIONS
// INSTRUCTION_CLEAR_ACTIONS
// INSTRUCTION_WRITE_ACTIONS
// INSTRUCTION_WRITE_METADATA
// INSTRUCTION_GOTO_TABLE

static void 
execute_instructions(struct instruction_set *is, uint8_t *table_id, struct flow_key *flow, struct action_set *as) {

    if (instruction_is_active(is, INSTRUCTION_APPLY_ACTIONS)){

    }

    if (instruction_is_active(is, INSTRUCTION_CLEAR_ACTIONS)){
        action_set_clean(as);
    }

    if (instruction_is_active(is, INSTRUCTION_WRITE_ACTIONS)){
        action_set_merge(as, &is->write_act.actions);
    }
    
    if (instruction_is_active(is, INSTRUCTION_WRITE_METADATA)){
        flow->metadata = is->write_meta.metadata;
    }

    if (instruction_is_active(is, INSTRUCTION_GOTO_TABLE)){
        *table_id = is->gt_table.table_id;
    }
}


/* The match can be modified by an action */
/* Return is a list of ports/ NULL or -1 */
void 
dp_handle_flow(struct datapath *dp, uint64_t time, uint64_t pkt_cnt, uint64_t byte_cnt, struct flow_key *match)
{
    /* Get the input port and update rx counters*/
    uint8_t table;
    struct flow *f;
    uint32_t in_port = match->in_port;
    struct port *p = dp_port(dp, in_port);
    if (p != NULL) {
        struct action_set acts;
        action_set_init(&acts);
        p->stats.rx_packets += byte_cnt;
        p->stats.rx_bytes += pkt_cnt;
        /* Enter pipeline */
        for(table = 0; table < MAX_TABLES; ++table){
            f = flow_table_lookup(dp->tables[table], match, time);
            if (f != NULL){
                /* Cut the packet and byte count if flow lasts longer than
                    remotion by hard timeout */
                
                /* Increase the flow counters */
                f->pkt_cnt += pkt_cnt;
                f->byte_cnt += byte_cnt;
                /* Execute instructions */
                execute_instructions(&f->insts, &table, match, &acts);
            }
        }
        /* Execute action set */ 
    }
}

uint64_t 
dp_uuid(const struct datapath* dp)
{
    return dp->base.uuid;
}


uint64_t 
dp_id(const struct datapath* dp)
{
    return dp->dp_id;
}
