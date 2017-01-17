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
    // free(topo);
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

struct topology from_json(char *json_file)
{
    size_t s;
    struct parsed_topology ptopo;
    struct topology topo;
    char *json = file_to_string(json_file, &s);
    if (json != NULL) {
        parse_topology(json, s, &ptopo);
        topology_init(&topo);
        topology_from_ptopo(&topo, &ptopo);
    }
    return topo;
}