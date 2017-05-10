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
    topo->n_dps = 0;
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
topology_add_datapath(struct topology *topo, struct datapath* dp)
{
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
topology_add_link(struct topology *t, uint64_t uuidA, uint64_t uuidB, uint32_t portA, uint32_t portB, uint32_t bw, uint32_t latency, bool directed)
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
topology_next_hop(const struct topology *topo, const uint64_t orig_uuid, const uint32_t orig_port, uint64_t *dst_uuid, uint32_t *dst_port, uint32_t *latency)
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

    /* Clean links */
    HASH_ITER(hh, topo->links, lcurr, ltmp) {
        HASH_DEL(topo->links, lcurr);  
        free(lcurr);
    }
    /* Clean Datapaths */
    HASH_ITER(hh, topo->nodes, cur_node, tmp) {
        HASH_DEL(topo->nodes, cur_node);  
        if (cur_node->type == DATAPATH){
            dp_destroy((struct datapath*) cur_node);    
        }
        else if (cur_node->type == HOST){
            host_destroy((struct host*) cur_node);    
        }
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

static
void topology_from_ptopo(struct topology* topo, struct parsed_topology* ptopo)
{
    size_t i;
    /* Create Datapaths */
    for (i = 0; i < ptopo->ndps; ++i){
        struct datapath *dp = dp_new(ptopo->dps[i]);
        uint8_t mac[6] = {0,0,0,0,0,1};
        dp_add_port(dp, 1, mac, 1000000, 1000000);
        dp_add_port(dp, 2, mac, 1000000, 1000000);
        topology_add_datapath(topo, dp);
    }
    /* Create links */
    for (i = 0; i < ptopo->nlinks; ++i){
        topology_add_link(topo, ptopo->links[i].switchX, ptopo->links[i].switchY, ptopo->links[i].portX, ptopo->links[i].portY, ptopo->links[i].delay, ptopo->links[i].bw, false);
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

uint32_t topology_links_num(const struct topology *topo)
{
    return topo->n_links;
}

struct node* topology_nodes(const struct topology *topo)
{
    return topo->nodes;
}
