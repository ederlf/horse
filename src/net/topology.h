/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef TOPOLOGY_H
#define TOPOLOGY_H 1

#include "datapath.h"
#include "host.h"
#include "router.h"
#include "lib/util.h"

/* Total number of possible datapaths
*  UINT32_MAX == 2 ^ 16 datapaths 
*/
#define MAX_DPS UINT16_MAX 

/* 
*   Pair of node uuid and port. 
*   Key of the hash table of links.
*/

struct node_port_pair {
    uint64_t uuid;
    uint32_t port;
};

/* TODO: Possible split link on its own interface */
struct link {
    struct node_port_pair node1;
    struct node_port_pair node2;
    uint32_t latency;               /* Latency in microseconds */
    uint32_t bandwidth;
    UT_hash_handle hh;  
};

/* Access datapaths by the dpid */
struct dp_node {
    uint64_t dp_id;
    struct datapath *dp;
    UT_hash_handle hh;
};

/* Access routers by the router_id */
struct router_node {
    uint32_t router_id;
    struct router *rt;
    UT_hash_handle hh;
};

struct host_node {
    uint64_t uuid;
    struct host *h;
    UT_hash_handle hh;
};

struct topology;

struct topology* topology_new(void);

void topology_add_router_to_map(struct topology *topo, struct router *r);
void topology_add_router(struct topology *topo, struct router *r);
void topology_add_datapath(struct topology *topo, struct datapath *dp);
void topology_add_host(struct topology *topo, struct host *h);

void topology_add_link(struct topology *t, uint64_t uuidA, 
                       uint64_t uuidB, uint32_t portA, uint32_t portB,
                       uint32_t bw, uint32_t latency, bool directed);

bool topology_next_hop(const struct topology *topo, const uint64_t orig_uuid,
                       const uint32_t orig_port, uint64_t *dst_uuid, 
                       uint32_t *dst_port, uint32_t *latency);

void topology_destroy(struct topology *topo);

struct node* topology_node(const struct topology *topo, uint64_t uuid);

struct datapath* topology_datapath_by_dpid(const struct topology *topo,
                                           uint64_t dp_id);

struct router* topology_router_by_id(const struct topology *topo,
                                       uint32_t router_id);

struct topology* from_json(char *json_file);

/* Get struct members */
uint32_t topology_dps_num(const struct topology *topo);

uint32_t topology_routers_num(const struct topology *topo);

uint32_t topology_links_num(const struct topology *topo);

struct node* topology_nodes(const struct topology *topo);

struct dp_node *topology_datapaths(const struct topology *topo);

struct router_node *topology_routers(const struct topology *topo);

struct host_node *topology_hosts(const struct topology *topo);

struct link* topology_links(const struct topology *topo);

#endif /* TOPOLOGY_H */