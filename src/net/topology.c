/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */


#include "topology.h"
#include "lib/json_topology.h"

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

/* Represents the network topology */
struct topology {
    struct node *nodes;             /* Hash table of network nodes. */
    struct link *links;             /* Hash table of links */
    struct dp_node *dps;            /* Access datapath nodes by dpid */
    struct router_node *routers;    /* Access routers by the router_id */
    uint32_t degree[MAX_DPS];       /* number of links connected to dps. */ 
    uint32_t n_dps;                 /* Number of datapaths. */
    uint32_t n_routers;             /* Number of routers. */
    uint32_t n_hosts;               /* Number of hosts. */
    uint32_t n_links;               /* Number of links. */
};

static void
topology_init(struct topology* topo)
{
    topo->nodes = NULL;
    topo->dps = NULL;
    topo->n_dps = 0;
    topo->n_routers = 0;
    topo->n_hosts = 0;
    topo->n_links = 0;
    topo->links = NULL;
}

struct topology* topology_new(void)
{
    struct topology *topo = xmalloc(sizeof(struct topology));
    topology_init(topo);
    return topo;
}

void 
topology_add_router_to_map(struct topology *topo, struct router *r)
{
    struct router_node *rn = xmalloc(sizeof (struct router_node));
    rn->router_id = router_id(r);
    rn->rt = r;
    HASH_ADD(hh, topo->routers, router_id, sizeof(uint32_t), rn);  
}

/* One function for each type is necessary because of the Python binding */
void 
topology_add_router(struct topology *topo, struct router *r)
{
    HASH_ADD(hh, topo->nodes, uuid, sizeof(uint64_t), (struct node*) r);
    topo->n_routers++;  
}

void 
topology_add_datapath(struct topology *topo, struct datapath* dp)
{
    struct dp_node *dn = xmalloc(sizeof (struct dp_node));
    dn->dp_id = dp_id(dp);
    dn->dp = dp;
    HASH_ADD(hh, topo->dps, dp_id, sizeof(uint64_t), dn);
    HASH_ADD(hh, topo->nodes, uuid, sizeof(uint64_t), (struct node*) dp);
    topo->n_dps++;
}

void 
topology_add_host(struct topology *topo, struct host *h)
{
    HASH_ADD(hh, topo->nodes, uuid, sizeof(uint64_t), (struct node*) h);
    topo->n_hosts++;
}

void 
topology_add_link(struct topology *t, uint64_t uuidA, uint64_t uuidB,
                  uint32_t portA, uint32_t portB, uint32_t bw, 
                  uint32_t latency, bool directed)
{
    struct node *dpA, *dpB;
    struct link *l;
    dpA = dpB = NULL;
    /* Check if A and B exist. */
    HASH_FIND(hh, t->nodes, &uuidA, sizeof(uint64_t), dpA);
    if (dpA == NULL){
        /* TODO: return error message. */
        return;
    }
    HASH_FIND(hh, t->nodes, &uuidB, sizeof(uint64_t), dpB);
    if (dpB == NULL){
        /* TODO: return error message. */
        return;
    }
    /* Fill link configuration. */
    l = (struct link*) xmalloc(sizeof(struct link));
    memset(l, 0x0, sizeof(struct link));

    l->node1.port = portA;
    l->node1.uuid = dpA->uuid;
    l->node2.port = portB;
    l->node2.uuid = dpB->uuid;
    l->latency = latency;
    l->bandwidth = bw;
    HASH_ADD(hh, t->links, node1, sizeof(struct node_port_pair), l);
    /* Insert backwards. */
    if (directed == false){
        topology_add_link(t, uuidB, uuidA, portB, portA, bw, latency, true);
    }
    else {
        t->n_links++;
    }
}

bool
topology_next_hop(const struct topology *topo, const uint64_t orig_uuid,
                  const uint32_t orig_port, uint64_t *dst_uuid, 
                  uint32_t *dst_port, uint32_t *latency)
{
    struct link *l;
    struct node_port_pair np;
    memset(&np, 0x0, sizeof(struct node_port_pair));
    np.uuid = orig_uuid;
    np.port = orig_port;
    HASH_FIND(hh, topo->links, &np, sizeof(struct node_port_pair), l);
    if (l != NULL){
        *dst_uuid = l->node2.uuid;
        *dst_port = l->node2.port;
        *latency = l->latency;
        return true;
    }
    return false;
}

/* Clean the dynamically allocated members of a topology 
*  As there should be a single instance of topology allocated 
*  in the stack it is not necessary to free topo.
*/

void 
topology_destroy(struct topology *topo)
{
    struct node *cur_node, *tmp;
    struct link *ltmp, *lcurr;
    struct dp_node *dncur, *dntmp;
    struct router_node *rncur, *rntmp;
    /* Clean links */
    HASH_ITER(hh, topo->links, lcurr, ltmp) {
        HASH_DEL(topo->links, lcurr);  
        free(lcurr);
    }
    /* Clean Nodes */
    HASH_ITER(hh, topo->nodes, cur_node, tmp) {
        HASH_DEL(topo->nodes, cur_node);  
        if (cur_node->type == DATAPATH){
            dp_destroy((struct datapath*) cur_node);    
        }
        else if (cur_node->type == HOST){
            host_destroy((struct host*) cur_node);    
        }
        else if (cur_node->type == ROUTER){
            router_destroy((struct router*) cur_node);    
        }
    }
    /* Clean datapath map */
    HASH_ITER(hh, topo->dps, dncur, dntmp) {
        HASH_DEL(topo->dps, dncur);  
        free(dncur);
    }
    HASH_ITER(hh, topo->routers, rncur, rntmp) {
        HASH_DEL(topo->routers, rncur);  
        free(rncur);
    }
    free(topo);
}

struct node* 
topology_node(const struct topology *topo, uint64_t uuid)
{
    struct node *n = NULL;
    HASH_FIND(hh, topo->nodes, &uuid, sizeof(uint64_t), n);
    return n;
}

struct datapath* 
topology_datapath_by_dpid(const struct topology *topo, uint64_t dp_id)
{
    struct dp_node *dn;
    HASH_FIND(hh, topo->dps, &dp_id, sizeof(uint64_t), dn);
    return dn->dp;
}

struct router*
topology_router_by_id(const struct topology *topo, uint32_t router_id)
{
    struct router_node *rn;
    HASH_FIND(hh, topo->routers, &router_id, sizeof(uint32_t), rn);
    return rn->rt;
}

static
void topology_from_ptopo(struct topology* topo, struct parsed_topology* ptopo)
{
    size_t i;
    /* Create Datapaths */
    for (i = 0; i < ptopo->ndps; ++i){
        struct datapath *dp = dp_new(ptopo->dps[i], "127.0.0.1", 6653);
        uint8_t mac[6] = {0,0,0,0,0,1};
        dp_add_port(dp, 1, mac, 1000000, 1000000);
        dp_add_port(dp, 2, mac, 1000000, 1000000);
        topology_add_datapath(topo, dp);
    }
    /* Create links */
    for (i = 0; i < ptopo->nlinks; ++i){
        topology_add_link(topo, ptopo->links[i].switchX,
                          ptopo->links[i].switchY, ptopo->links[i].portX, 
                          ptopo->links[i].portY, ptopo->links[i].delay, 
                          ptopo->links[i].bw, false);
    }
}

struct topology* from_json(char *json_file)
{
    size_t s;
    struct parsed_topology ptopo;
    struct topology* topo = topology_new();
    char *json = file_to_string(json_file, &s);
    if (json != NULL) {
        parse_topology(json, s, &ptopo);
        topology_from_ptopo(topo, &ptopo);
    }
    return topo;
}

/* Get struct members */
uint32_t 
topology_dps_num(const struct topology *topo)
{
    return topo->n_dps;
}

uint32_t 
topology_routers_num(const struct topology *topo)
{
    return topo->n_routers;
}

uint32_t topology_links_num(const struct topology *topo)
{
    return topo->n_links;
}

struct node* topology_nodes(const struct topology *topo)
{
    return topo->nodes;
}
