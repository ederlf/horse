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

/* Representation of a link of the network. 
*   
*/
struct link {
    uint32_t orig_port; /* Port number of the datapath.           */
    uint32_t end_uuid;  /* Id of the node connected.              */
    uint32_t end_port;  /* Port number of the datapath connected. */
    uint32_t latency;   /* Latency of the link in ms.             */
    uint32_t bandwidth; /* Maximum bandwidth of the link.         */
    struct link* next;  /* Next link in a list.                   */
};


/* Represents the network topology */
struct topology {
    struct node *nodes;             /* Hash map of network nodes. 
                                     * The datapath id is the key. */
    struct link* links[MAX_DPS];    /* List of link edges of the topology. */
    uint32_t degree[MAX_DPS];       /* number of links connected to dps. */ 
    uint32_t n_datapaths;           /* Number of datapaths. */
    uint32_t n_routers;             /* Number of routers */
    uint32_t n_links;               /* Number of links. */
};

void
topology_init(struct topology* topo)
{
    int i;
    topo->nodes = NULL;
    topo->n_datapaths = 0;
    topo->n_routers = 0;
    topo->n_links = 0;
    for (i = 0; i < MAX_DPS; ++i){
        topo->degree[i] = 0;
        topo->links[i] = NULL;
    }
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
    topo->n_datapaths++;
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
    l->bandwidth = bw;
    l->orig_port = portA;
    l->end_port = portB;
    l->latency = latency;
    l->end_uuid = dpB->uuid;
    l->next = t->links[dpA->uuid];

    t->links[dpA->uuid] = l;
    t->degree[dpA->uuid]++;
    /* Insert backwards. */
    if (directed == false){
        topology_add_link(t, uuidB, uuidA, portB, portA, bw, latency, true);
    }
    else {
        t->n_links++;
    }
}

/* Clean the dynamically allocated members of a topology 
*  As there should be a single instance of topology allocated 
*  in the stack it is not necessary to free topo.
*/

void 
topology_destroy(struct topology *topo)
{
    struct node *cur_node, *tmp;
    struct link *ltmp, *curr;
    size_t i;
    for (i = 0; i < topo->n_datapaths + topo->n_routers; ++i){
        if (topo->degree[i] > 0){
            curr = topo->links[i];
            while(curr){
                ltmp = curr;
                curr =  ltmp->next;
                free(ltmp);
            }
        }
    }
    HASH_ITER(hh, topo->nodes, cur_node, tmp) {
        HASH_DEL(topo->nodes, cur_node);  
        if (cur_node->type == DATAPATH){
            dp_destroy((struct datapath*) cur_node);    
        }
    }
    free(topo);
}

struct node* 
topology_node(struct topology *topo, uint64_t uuid)
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
topology_dps_num(struct topology *topo)
{
    return topo->n_datapaths;
}

uint32_t topology_links_num(struct topology *topo)
{
    return topo->n_links;
}