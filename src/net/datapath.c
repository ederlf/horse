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
#include "dp_actions.h"
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

static 
void execute_action_set(struct action_set *as, struct netflow *flow, struct out_port *out_ports){

    struct action *act;
    enum action_set_order type;
    /* Loop through the enum */
    for (type = ACT_METER; type < ACT_OUTPUT; ++type) {
        act = action_set_action(as, type);
        if(act){
            execute_action(act, flow, out_ports);
            continue;
        }
    }
}

static void 
execute_instructions(struct instruction_set *is, uint8_t *table_id, struct netflow *flow, struct action_set *as) {

    if (instruction_is_active(is, INSTRUCTION_APPLY_ACTIONS)){

    }

    if (instruction_is_active(is, INSTRUCTION_CLEAR_ACTIONS)){
        action_set_clean(as);
    }

    if (instruction_is_active(is, INSTRUCTION_WRITE_ACTIONS)){
        action_set_merge(as, &is->write_act.actions);
    }
    
    if (instruction_is_active(is, INSTRUCTION_WRITE_METADATA)){
        flow->match.metadata = is->write_meta.metadata;
    }

    if (instruction_is_active(is, INSTRUCTION_GOTO_TABLE)){
        *table_id = is->gt_table.table_id;
    }
}

/* The match can be modified by an action */
/* Return is a list of ports or NULL in case it is dropped*/
void 
dp_handle_flow(struct datapath *dp, struct netflow *flow)
{
    /* Get the input port and update rx counters*/
    uint8_t table;
    struct flow *f;
    struct out_port *out_ports = NULL;
    uint32_t in_port = flow->match.in_port;
    struct port *p = dp_port(dp, in_port);
    if (p != NULL) {
        struct action_set acts;
        action_set_init(&acts);
        p->stats.rx_packets += flow->byte_cnt;
        p->stats.rx_bytes += flow->pkt_cnt;
        /* Enter pipeline */
        for(table = 0; table < MAX_TABLES; ++table){
            f = flow_table_lookup(dp->tables[table], &flow->match, flow->start_time);
            if (f != NULL){
                /* Cut the packet and byte count if flow lasts longer than
                    remotion by hard timeout */
                
                /* Increase the flow counters */
                f->pkt_cnt += flow->pkt_cnt;
                f->byte_cnt += flow->byte_cnt;
                /* Execute instructions */
                execute_instructions(&f->insts, &table, flow, &acts);
            }
        }
        /* Execute action set */ 
        execute_action_set(&acts, flow, out_ports);
        if (out_ports != NULL){

        }
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
