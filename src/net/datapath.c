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
#include "lib/openflow.h"
#include "lib/of_unpack.h"
#include "lib/of_pack.h"
#include <cfluid/of_settings.h>
#include <uthash/utlist.h>

/* Definition of a switch of the network */
struct datapath {
    struct node base;
    uint64_t dp_id; /* Unique identification number of a switch in network*/
    struct flow_table *tables[MAX_TABLES];
    struct of_settings *dp_settings;
};

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
    dp->base.recv_netflow = dp_recv_netflow;
    dp->base.send_netflow = dp_send_netflow;
    /* TODO: Remove redundant dpid field */
    dp->dp_id = dp_id;
    /* Create flow tables*/
    for (i = 0; i < MAX_TABLES; ++i) {
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
    for (i = 0; i < MAX_TABLES; ++i) {
        flow_table_destroy(dp->tables[i]);
    }
    node_destroy_ports(&dp->base);
    // of_settings_destroy(dp->dp_settings);
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
    LL_FOREACH(al->actions, act_elem) {
        execute_action(&act_elem->act, flow);
    }
}

static void
execute_action_set(struct action_set *as, struct netflow *flow) {

    enum action_set_order type;
    /* Loop through the enum */
    for (type = ACT_METER; type <= ACT_OUTPUT; ++type) {
        struct action *act = action_set_action(as, type);
        if (act) {
            execute_action(act, flow);
        }
    }
}

static void
execute_instructions(struct instruction_set *is, uint8_t *table_id, struct netflow *flow, struct action_set *as) {

    if (instruction_is_active(is, INSTRUCTION_APPLY_ACTIONS)) {
        execute_action_list(&is->apply_act.actions, flow);
    }

    if (instruction_is_active(is, INSTRUCTION_CLEAR_ACTIONS)) {
        action_set_clean(as);
    }

    if (instruction_is_active(is, INSTRUCTION_WRITE_ACTIONS)) {
        action_set_merge(as, &is->write_act.actions);
    }

    if (instruction_is_active(is, INSTRUCTION_WRITE_METADATA)) {
        flow->match.metadata = is->write_meta.metadata;
    }

    if (instruction_is_active(is, INSTRUCTION_GOTO_TABLE)) {
        *table_id = is->gt_table.table_id;
    }
}

/* The match can be modified by an action */
/* Return is a list of ports or NULL in case it is dropped*/
bool
dp_recv_netflow(struct node *n, struct netflow **nf)
{
    /* Get the input port and update rx counters*/
    uint8_t table_id;
    struct flow *f;
    /* Buffering to be implemented */
    struct datapath *dp = (struct datapath*) n;
    (*nf)->metadata.buffer_id = OFP_NO_BUFFER;
    uint32_t in_port = (*nf)->match.in_port;
    struct port *p = dp_port(dp, in_port);
    if (p != NULL) {
        struct flow_table *table;
        struct action_set acts;
        action_set_init(&acts);
        p->stats.rx_bytes += (*nf)->byte_cnt;
        p->stats.rx_packets += (*nf)->pkt_cnt;
        // printf("receiving Flow %s from %x to %x  at %ld\n", p->name, (*nf)->match.ipv4_src, (*nf)->match.ipv4_dst, (*nf)->start_time);
        /* Reset metadata */
        (*nf)->match.metadata = 0;
        /* Enter pipeline */
        table = dp->tables[0];
        table_id = (*nf)->metadata.table_id = 0;
        while (table) {
            f = flow_table_lookup(table, &(*nf)->match, (*nf)->start_time);
            table = NULL;
            if (f != NULL) {
                uint8_t next_table_id = 0;
                /* TODO: Cut the packet and byte count if flow lasts longer than remotion by hard timeout */
                /* Increase the flow counters */
                f->pkt_cnt += (*nf)->pkt_cnt;
                f->byte_cnt += (*nf)->byte_cnt;
                (*nf)->metadata.cookie = f->cookie;
                /* Execute instructions */
                execute_instructions(&f->insts, &next_table_id, *nf, &acts);
                if (next_table_id > table_id) {
                    table_id = (*nf)->metadata.table_id = next_table_id;
                    table = dp->tables[table_id];
                }
                else {
                    /* Execute action and clean */
                    execute_action_set(&acts, *nf);
                    action_set_clean(&acts);
                    /* It only makes sense to continue later if it 
                       will be forwarded.
                    */
                    if ((*nf)->out_ports){
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void
dp_send_netflow(struct node *n, struct netflow *flow, uint32_t out_port)
{
    struct datapath *dp = (struct datapath*) n;
    struct port *p = dp_port(dp, out_port);
    if (p != NULL) {
        uint8_t upnlive = (p->config & PORT_UP) && (p->state & PORT_LIVE);
        if (upnlive) {
            // printf("Sending flow %s Time %ld\n", p->name, flow->start_time);
            p->stats.tx_packets += flow->pkt_cnt;
            p->stats.tx_bytes += flow->byte_cnt;
           
        }
    }
}

void 
dp_create_flood(struct datapath *dp, struct netflow *nf)
{
    struct port *p, *tmp_port;
    HASH_ITER(hh, dp_ports(dp), p, tmp_port) {
        if (p->port_id != nf->match.in_port) {
            struct out_port *new_port = xmalloc(sizeof(struct out_port));
            new_port->port = p->port_id;
            LL_APPEND(nf->out_ports, new_port);
        }
    }
}

of_object_t*
dp_handle_flow_mod(const struct datapath *dp,
                   of_object_t *obj, uint64_t time)
{
    struct flow_table *ft;
    struct flow *f = flow_new();
    unpack_flow_mod(obj, f);

    /*  Redundant switch here,
     *  but it is better than repeat code to handle the different
     *  objects that loci has for the flow mod.
     *  If there is a performance need, it is something to be revisited.
    */
    ft = dp->tables[f->table_id];
    switch (obj->object_id) {
    case OF_FLOW_ADD: {
        add_flow(ft, f, time);
        break;
    }
    case OF_FLOW_MODIFY: {
        modify_flow(ft, f, false, time);
    }
    case OF_FLOW_MODIFY_STRICT: {
        modify_flow(ft, f, true, time);
        break;
    }
    case OF_FLOW_DELETE: {
        delete_flow(ft, f, false, time);
        break;
    }
    case OF_FLOW_DELETE_STRICT: {
        delete_flow(ft, f, true, time);
        break;
    }
    default: {
        break;
    }
    }
    return NULL;
}

of_object_t*
dp_handle_port_stats_req(const struct datapath *dp,
                         of_object_t *obj)
{
    uint32_t xid;
    of_port_no_t port_id;
    of_port_stats_reply_t *reply;
    of_port_stats_request_t *req = (of_port_stats_request_t*) obj;
    of_port_stats_entry_t *port_entry = NULL;
    of_list_port_stats_entry_t *port_list = NULL;

    if ((reply = of_port_stats_reply_new(obj->version)) == NULL) {
        fprintf(stderr, "%s\n", "Failed to create port stats reply object");
        return NULL;
    }

    of_port_stats_request_xid_get(req, &xid);
    of_port_stats_reply_xid_set(reply, xid);

    port_entry = of_port_stats_entry_new(obj->version);
    if (port_entry == NULL) {
        fprintf(stderr, "%s\n", "Failed to create a port entry object");
        return NULL;
    }

    port_list = of_list_port_stats_entry_new(obj->version);
    if (port_list == NULL) {
        fprintf(stderr, "%s\n", "Failed to create a port status list object");
        of_port_stats_entry_delete(port_entry);
        return NULL;
    }

    of_port_stats_request_port_no_get(req, &port_id);

    /* Return all interfaces */
    if (port_id == OFPP_ANY) {
        struct port *cur_port, *tmp;
        HASH_ITER(hh, dp->base.ports, cur_port, tmp) {
            /* TODO: Put it in a function */
            of_port_stats_entry_port_no_set(port_entry, cur_port->port_id);
            of_port_stats_entry_duration_sec_set(port_entry, 0);
            of_port_stats_entry_duration_nsec_set(port_entry, 0);
            of_port_stats_entry_rx_packets_set(port_entry, cur_port->stats.rx_packets);
            of_port_stats_entry_tx_packets_set(port_entry, cur_port->stats.tx_packets);
            of_port_stats_entry_rx_bytes_set(port_entry, cur_port->stats.rx_bytes);
            of_port_stats_entry_tx_bytes_set(port_entry, cur_port->stats.tx_bytes);
            of_port_stats_entry_rx_dropped_set(port_entry, 0);
            of_port_stats_entry_tx_dropped_set(port_entry, 0);
            of_port_stats_entry_rx_errors_set(port_entry, 0);
            of_port_stats_entry_tx_errors_set(port_entry, 0);
            of_port_stats_entry_rx_frame_err_set(port_entry, 0);
            of_port_stats_entry_rx_over_err_set(port_entry, 0);
            of_port_stats_entry_rx_crc_err_set(port_entry, 0);
            of_port_stats_entry_collisions_set(port_entry, 0);
            of_list_port_stats_entry_append(port_list, port_entry);
        }
    }
    else {
        struct port *p = dp_port(dp, port_id);
        of_port_stats_entry_port_no_set(port_entry, p->port_id);
        of_port_stats_entry_duration_sec_set(port_entry, 0);
        of_port_stats_entry_duration_nsec_set(port_entry, 0);
        of_port_stats_entry_rx_packets_set(port_entry, p->stats.rx_packets);
        of_port_stats_entry_tx_packets_set(port_entry, p->stats.tx_packets);
        of_port_stats_entry_rx_bytes_set(port_entry, p->stats.rx_bytes);
        of_port_stats_entry_tx_bytes_set(port_entry, p->stats.tx_bytes);
        of_port_stats_entry_rx_dropped_set(port_entry, 0);
        of_port_stats_entry_tx_dropped_set(port_entry, 0);
        of_port_stats_entry_rx_errors_set(port_entry, 0);
        of_port_stats_entry_tx_errors_set(port_entry, 0);
        of_port_stats_entry_rx_frame_err_set(port_entry, 0);
        of_port_stats_entry_rx_over_err_set(port_entry, 0);
        of_port_stats_entry_rx_crc_err_set(port_entry, 0);
        of_port_stats_entry_collisions_set(port_entry, 0);
        of_list_port_stats_entry_append(port_list, port_entry);
    }

    if (of_port_stats_reply_entries_set(reply, port_list) < 0) {
        fprintf(stderr, "%s\n", "Failure to add list of ports to stats reply" );
        return NULL;
    }

    of_port_stats_entry_delete(port_entry);
    of_list_port_stats_entry_delete(port_list);
    return reply;
}

static void dp_get_stats_flows(const struct datapath *dp, 
                               struct ofl_flow_stats_req *req, struct flow ***flows, size_t *flow_count, 
                               uint64_t time)
{
    if (req->table_id == OFPTT_ALL) {
        size_t i;
        for (i = 0; i < MAX_TABLES; i++) {
            flow_table_stats(dp->tables[i], req, flows, flow_count, time);
        }
    } else {
        flow_table_stats(dp->tables[req->table_id], req, flows,
                         flow_count, time);
    }  
}

of_object_t *
dp_handle_flow_stats_req(const struct datapath *dp, of_object_t* obj,
                         uint64_t time)
{
    uint32_t xid;
    size_t flow_count = 0;
    struct ofl_flow_stats_req req;
    of_flow_stats_reply_t *reply = NULL;
    unpack_flow_stats_request(obj, &req);
    of_flow_stats_request_xid_get(obj, &xid);

    struct flow **flows = xmalloc(sizeof(struct flow*));
    memset(&req, 0x0, sizeof(struct ofl_flow_stats_req));
    dp_get_stats_flows(dp, &req, &flows, &flow_count, time);
    /* Pack and return */
    if (flow_count > 0) {
       reply = pack_flow_stats_reply(flows, xid, flow_count);
    }
    free(flows);
    return reply;
}

of_object_t* 
dp_handle_aggregate_stats_req(const struct datapath *dp,
                              of_object_t* obj, uint64_t time)
{
    uint32_t xid;
    size_t flow_count = 0;
    struct ofl_flow_stats_req req;
    of_aggregate_stats_reply_t *reply = NULL;
    unpack_aggregate_stats_request(obj, &req);
    of_aggregate_stats_request_xid_get(obj, &xid);

    struct flow **flows = xmalloc(sizeof(struct flow*));
    memset(&req, 0x0, sizeof(struct ofl_flow_stats_req));

    dp_get_stats_flows(dp, &req, &flows, &flow_count, time);

    /* Pack and return */
    if (flow_count > 0) {
       reply = pack_aggregate_stats_reply(flows, xid, flow_count);
    }
    free(flows);
    return reply;

}

of_object_t*
dp_handle_port_desc(const struct datapath *dp, of_object_t* obj)
{
    uint32_t xid;
    of_port_desc_t *of_port_desc = NULL;
    of_list_port_desc_t *of_list_port_desc = NULL;
    of_mac_addr_t mac;
    struct port *cur_port, *tmp;

    of_port_desc_stats_request_t *req = (of_port_desc_stats_request_t*) obj;
    of_port_desc_stats_reply_t *reply;

    if ((reply = of_port_desc_stats_reply_new(obj->version)) == NULL) {
        fprintf(stderr, "%s\n", "Failed to create port desc reply object");
        return NULL;
    }

    of_port_desc_stats_request_xid_get(req, &xid);
    of_port_desc_stats_reply_xid_set(reply, xid);

    /* Allocates memory for of_port_desc */
    of_port_desc = of_port_desc_new(obj->version);
    if (of_port_desc == NULL) {
        fprintf(stderr, "%s\n", "Failed to create a port desc object");
        return NULL;
    }

    /* Allocates memory for of_list_port_desc */
    of_list_port_desc = of_list_port_desc_new(obj->version);
    if (of_list_port_desc == NULL)
    {
        fprintf(stderr, "%s\n", "Failed to create a list port desc object");
        of_port_desc_delete(of_port_desc);
        return NULL;
    }

    HASH_ITER(hh, dp->base.ports, cur_port, tmp) {
        of_port_desc_port_no_set(of_port_desc, cur_port->port_id);
        memcpy(&mac, cur_port->eth_address, ETH_LEN);
        of_port_desc_hw_addr_set(of_port_desc, mac);
        of_port_desc_name_set(of_port_desc, cur_port->name);
        of_port_desc_config_set(of_port_desc, cur_port->config);
        of_port_desc_state_set(of_port_desc, cur_port->state);
        of_port_desc_curr_set(of_port_desc, cur_port->curr);
        of_port_desc_advertised_set(of_port_desc, cur_port->advertised);
        of_port_desc_supported_set(of_port_desc, cur_port->supported);
        of_port_desc_peer_set(of_port_desc, cur_port->peer);
        of_port_desc_curr_speed_set(of_port_desc, cur_port->curr_speed);
        of_port_desc_max_speed_set(of_port_desc, cur_port->max_speed);

        if (of_list_port_desc_append(of_list_port_desc, of_port_desc) < 0) {
            fprintf(stderr, "%s\n", "Failure adding port desc to list");
            return NULL;
        }
    }

    if (of_port_desc_stats_reply_entries_set(reply, of_list_port_desc) < 0) {
        fprintf(stderr, "%s\n", "Failure to add list of ports to desc stats reply" );
        return NULL;
    }

    of_port_desc_delete(of_port_desc);
    of_list_port_desc_delete(of_list_port_desc);
    // of_port_desc_stats_request_delete(req);
    return reply;
}

of_object_t*
dp_handle_pkt_out(struct datapath *dp, of_object_t *obj, struct netflow *nf, uint64_t time)
{
    struct action_list al;
    nf->start_time = time;
    action_list_init(&al);
    unpack_packet_out(obj, nf, &al);
    execute_action_list(&al, nf);
    /* TODO: Buffering */
    // printf("Sending packet_out\n");
    // dp_send_netflow((struct node*) dp, nf);
    action_list_clean(&al);
    UNUSED(dp);
    return NULL;
}

void dp_set_name(struct datapath* dp, char *name) {
    memcpy(dp->base.name, name, MAX_NODE_NAME);
}

char *dp_name(struct datapath *dp)
{
    return dp->base.name;
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

struct port* 
dp_ports(const struct datapath *dp)
{
    return dp->base.ports;
}

void 
dp_write_stats(const struct datapath *dp, uint64_t time, FILE *fp)
{
    struct port *p, *tmp;
    uint64_t total_tx = 0; 
    uint64_t total_rx = 0;
    uint64_t t = time / 1000000;
    HASH_ITER(hh, dp->base.ports, p, tmp) {
        uint64_t tx_rate, rx_rate;
        tx_rate = p->stats.tx_bytes - p->prev_stats.tx_bytes;
        rx_rate = p->stats.rx_bytes - p->prev_stats.rx_bytes;
        total_tx += tx_rate;
        total_rx += rx_rate;
        fprintf (fp, "%"PRIu64",%s,%"PRIu64",%"PRIu64"\n", t, p->name, tx_rate, rx_rate);
        p->prev_stats.tx_bytes = p->stats.tx_bytes;
        p->prev_stats.rx_bytes = p->stats.rx_bytes;
    }
    // fprintf (fp, "%"PRIu64";%s;%"PRIu64";%"PRIu64"\n", t, "total", total_tx, total_rx);
}