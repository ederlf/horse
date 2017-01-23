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
    uint32_t conn_dp;   /* Id of the datapath connected.          */
    uint32_t orig_port; /* Port number of the datapath.           */
    uint32_t end_port;  /* Port number of the datapath connected. */
    uint32_t latency;   /* Latency of the link in ms.             */
    uint32_t bandwidth; /* Maximum bandwidth of the link.         */
    struct link* next;  /* Next link in a list.                   */
};


/* Represents the network topology */
struct topology {
    struct datapath *dps;           /* Hash map of switches. 
                                     * The datapath id is the key. */
    struct link* links[MAX_DPS];    /* List of link edges of the topology. */
    uint32_t degree[MAX_DPS];       /* number of links connected to dps. */ 
    uint32_t ndatapaths;             /* Number of datapaths. */
    uint32_t nlinks;                 /* Number of links. */
};

void
topology_init(struct topology* topo)
{
    int i;
    topo->dps = NULL;
    topo->ndatapaths = 0;
    topo->ndatapaths = 0;
    topo->nlinks = 0;
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
topology_add_switch(struct topology *topo, struct datapath* dp)
{
    HASH_ADD(hh, topo->dps, dp_id, sizeof(uint64_t), dp);
    topo->ndatapaths++;
}

void 
topology_add_link(struct topology *t, uint64_t dpidA, uint64_t dpidB, uint32_t portA, uint32_t portB, uint32_t bw, uint32_t latency, bool directed)
{
    struct datapath *dpA, *dpB;
    struct link *l;
    dpA = dpB = NULL;
    /* Check if A and B exist. */
    HASH_FIND(hh, t->dps, &dpidA, sizeof(uint64_t), dpA);
    if (dpA == NULL){
        /* TODO: return error message. */
        return;
    }
    HASH_FIND(hh, t->dps, &dpidB, sizeof(uint64_t), dpB);
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
    l->conn_dp = dpB->uuid;
    l->next = t->links[dpA->uuid];

    t->links[dpA->uuid] = l;
    t->degree[dpA->uuid]++;
    /* Insert backwards. */
    if (directed == false){
        topology_add_link(t, dpidB, dpidA, portB, portA, bw, latency, true);
    }
    else {
        t->nlinks++;
    }
}

/* Clean the dynamically allocated members of a topology 
*  As there should be a single instance of topology allocated 
*  in the stack it is not necessary to free topo.
*/

void 
topology_destroy(struct topology *topo)
{
    struct datapath *current_dp, *tmp;
    struct link *ltmp, *curr;
    size_t i;
    for (i = 0; i < topo->ndatapaths; ++i){
        if (topo->degree[i] > 0){
            curr = topo->links[i];
            while(curr){
                ltmp = curr;
                curr =  ltmp->next;
                free(ltmp);
            }
        }
    }
    HASH_ITER(hh, topo->dps, current_dp, tmp) {
        HASH_DEL(topo->dps, current_dp);  
        dp_destroy(current_dp);
    }
    free(topo);
}

struct datapath* 
topology_datapath(struct topology *topo, uint64_t dpid)
{
    struct datapath *dp = NULL;
    HASH_FIND(hh, topo->dps, &dpid, sizeof(uint64_t), dp);
    return dp;
}

static
void topology_from_ptopo(struct topology* topo, struct parsed_topology* ptopo)
{
    size_t i;
    /* Create Datapaths */
    for (i = 0; i < ptopo->ndps; ++i){
        struct datapath *dp = dp_new(ptopo->dps[i]);
        topology_add_switch(topo, dp);
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
    return topo->ndatapaths;
}

uint32_t topology_links_num(struct topology *topo)
{
    return topo->nlinks;
}