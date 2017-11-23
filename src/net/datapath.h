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
#include <loci/loci.h>

/* Total number of possible ports
*  UINT32_MAX == 2 ^ 32 datapaths
*/
#define MAX_PORT_NUM UINT32_MAX /* 2 ^ 8 ports */

/* Total number of tables per datapath */
#define MAX_TABLES 128

struct datapath;

/* Access datapaths by the dpid */
struct dp_node {
    uint64_t dp_id;
    struct datapath *dp;
    UT_hash_handle hh;
};

struct datapath* dp_new(uint64_t dp_id, char *ip, int port);

void dp_destroy(struct datapath *dp);

void dp_add_port(struct datapath *dp, uint32_t port_id,
                 uint8_t eth_addr[ETH_LEN], uint32_t speed, uint32_t curr_speed);

struct port* dp_port(const struct datapath *dp, uint32_t port_id);

bool dp_recv_netflow(struct node *n, struct netflow **flow);

void dp_send_netflow(struct node *n, struct netflow *flow, uint32_t out_port);

void dp_create_flood(struct datapath *dp, struct netflow *nf);

of_object_t* dp_handle_flow_mod(const struct datapath *dp,
                                of_flow_modify_t *obj, uint64_t time);

of_object_t* dp_handle_port_stats_req(const struct datapath *dp,
                                      of_object_t *obj);

of_object_t* dp_handle_flow_stats_req(const struct datapath *dp,
                                      of_object_t* obj, uint64_t time);

of_object_t* dp_handle_aggregate_stats_req(const struct datapath *dp,
                                      of_object_t* obj, uint64_t time);

of_object_t* dp_handle_port_desc(const struct datapath *dp,
                                 of_object_t *obj);

of_object_t* dp_handle_pkt_out(struct datapath *dp, of_object_t *obj,
                               struct netflow *nf, uint64_t time);

/* Access functions*/
void dp_set_name(struct datapath *dp, char* name);

char* dp_name(struct datapath *dp);

uint64_t dp_uuid(const struct datapath* dp);

uint64_t dp_id(const struct datapath* dp);

struct port* dp_ports(const struct datapath *dp);

/* Just for tests */
struct flow_table *dp_flow_table(const struct datapath *dp, uint8_t table_id);

struct of_settings *dp_settings(const struct datapath *dp);

void dp_write_stats(const struct datapath *dp, uint64_t time, FILE *fp);
#endif /*DATAPATH_H */
