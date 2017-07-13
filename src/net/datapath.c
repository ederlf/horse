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
#include <cfluid/of_settings.h>
#include <uthash/utlist.h>


/* Definition of a switch of the network */
struct datapath {
    struct node base;
    uint64_t dp_id; /* Unique identification number of a switch in network*/ 
    struct flow_table *tables[MAX_TABLES];
    struct of_settings *dp_settings;
};

static void dp_recv_netflow(struct datapath *n, struct netflow *flow);
static void dp_send_netflow(struct datapath *n, struct netflow *flow);

/* Creates a new datapath. 
*
*  A datapath starts without any port assigned.
*  @ip is not the ip of the switch but of the controller it may connect. 
*/
struct datapath* 
dp_new(uint64_t dp_id, char *ip, int port)
{
    struct datapath *dp = xmalloc(sizeof(struct datapath));
    int i;
    node_init(&dp->base, DATAPATH);
    dp->base.handle_netflow = dp_handle_netflow;
    /* TODO: Remove redundant dpid field */
    dp->dp_id = dp_id;
    /* Create flow tables*/
    for (i = 0; i < MAX_TABLES; ++i){
        dp->tables[i] = flow_table_new(i);
    }
    dp->dp_settings = of_settings_new(ip, port, false);
    dp->dp_settings->datapath_id = dp_id;
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
dp_add_port(struct datapath *dp, uint32_t port_id, uint8_t eth_addr[ETH_LEN], uint32_t speed, uint32_t curr_speed)
{   
    node_add_port(&dp->base, port_id, eth_addr, speed, curr_speed);
}

/* Retrieve a datapath port */
struct port* 
dp_port(const struct datapath *dp, uint32_t port_id)
{
    struct port *p = node_port(&dp->base, port_id);
    return p;
}

static void 
execute_action_list(struct action_list *al, struct netflow *flow)
{
    struct action_list_elem *act_elem;
    LL_FOREACH(al->actions, act_elem){
        execute_action(&act_elem->act, flow);
    }
}


static void 
execute_action_set(struct action_set *as, struct netflow *flow){

    struct action *act;
    enum action_set_order type;
    /* Loop through the enum */
    for (type = ACT_METER; type <= ACT_OUTPUT; ++type) {
        act = action_set_action(as, type);
        if(act){
            execute_action(act, flow);
        }
    }
}

static void 
execute_instructions(struct instruction_set *is, uint8_t *table_id, struct netflow *flow, struct action_set *as) {

    if (instruction_is_active(is, INSTRUCTION_APPLY_ACTIONS)){
        execute_action_list(&is->apply_act.actions, flow);
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

void
dp_handle_netflow(struct node *n, struct netflow *flow){

    struct datapath *dp = (struct datapath*) n;
    /* There are no out ports, it is receiving */
    if (!flow->out_ports){
        dp_recv_netflow(dp, flow);
    }
    dp_send_netflow(dp, flow);
}

/* The match can be modified by an action */
/* Return is a list of ports or NULL in case it is dropped*/
void
dp_recv_netflow(struct datapath *dp, struct netflow *flow)
{
    /* Get the input port and update rx counters*/
    uint8_t table_id;
    struct flow_table *table;
    struct flow *f;
    
    uint32_t in_port = flow->match.in_port;
    struct port *p = dp_port(dp, in_port);
    if (p != NULL) {
        struct action_set acts;
        action_set_init(&acts);
        p->stats.rx_packets += flow->byte_cnt;
        p->stats.rx_bytes += flow->pkt_cnt;
        /* Reset metadata */
        flow->match.metadata = 0;
        /* Enter pipeline */
        table = dp->tables[0];
        table_id = 0;
        while(table){
            f = flow_table_lookup(table, &flow->match, flow->start_time);
            table = NULL;
            if (f != NULL){
                uint8_t next_table_id = 0;
                /* TODO: Cut the packet and byte count if flow lasts longer than remotion by hard timeout */
                /* Increase the flow counters */
                f->pkt_cnt += flow->pkt_cnt;
                f->byte_cnt += flow->byte_cnt;
                /* Execute instructions */
                execute_instructions(&f->insts, &next_table_id, flow, &acts);
                if (next_table_id > table_id){
                    table_id = next_table_id;
                    table = dp->tables[table_id];
                }
                else {
                     /* Execute action and clean */
                    execute_action_set(&acts, flow);
                    action_set_clean(&acts);
                }
            }
        }
    }
}

void
dp_send_netflow(struct datapath *dp, struct netflow *flow)
{   
    struct out_port *op;
    struct out_port *ports = flow->out_ports;
    LL_FOREACH(ports, op) {
        struct port *p = dp_port(dp, op->port);
        if (p != NULL) {
            uint8_t upnlive = (p->config & PORT_UP) && (p->state & PORT_LIVE);
            if (upnlive){
                p->stats.tx_packets += flow->pkt_cnt;
                p->stats.tx_bytes += flow->byte_cnt;
                if (dp->base.uuid == 1){
                    // printf("Port %d -- %ld Packets | %ld Bytes -- time %ld\n", op->port, p->stats.tx_packets, p->stats.tx_bytes, flow->start_time);   
                }
                /* Start time of the flow will be the same as the end */
                netflow_update_send_time(flow, p->curr_speed);
            }
        }
    }
}

/* TODO: better to pass a struct that represents a flow_mod */
void
dp_handle_flow_mod(const struct datapath *dp, uint8_t table_id, struct flow *f, uint64_t time)
{
    add_flow(dp->tables[table_id], f, time);
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

struct flow_table 
*dp_flow_table(const struct datapath *dp, uint8_t table_id)
{
    return dp->tables[table_id];
}

struct of_settings
*dp_settings(const struct datapath *dp)
{
    return dp->dp_settings;
}