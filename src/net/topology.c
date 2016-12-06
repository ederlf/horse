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

void 
topology_add_link(struct topology *t, uint64_t dpidA, uint64_t dpidB, uint32_t bw, uint32_t latency){
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
    l->latency = latency;
    l->conn_dp = dpB->uuid;
    l->next = t->links[dpA->uuid];

    t->links[dpA->uuid] = l;
    t->degree[dpA->uuid]++;
    /* Insert backwards. */
    topology_add_link(t, dpidB, dpidA, bw, latency);
    t->nlinks++;
}

void
topology_init(struct topology* topo){
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
topology_add_switch(struct topology *topo, struct datapath* dp){
    HASH_ADD(hh, topo->dps, dp_id, sizeof(uint64_t), dp);
    topo->ndatapaths++;
}

void 
topology_destroy(struct topology *topo){
    struct datapath *current_dp, *tmp;
    struct link *ltmp, *curr;
    size_t i;
    for (i = 0; i < topo->ndatapaths; ++i){
        if (topo->degree[i] > 0){
            curr = topo->links[i];
            while(curr){
               ltmp = curr;
               curr =  topo->links[i]->next;
               free(ltmp);
            }
        }
    }
    HASH_ITER(hh, topo->dps, current_dp, tmp) {
        HASH_DEL(topo->dps, current_dp);  
        free(current_dp);
    }

    // free(topo);
}
