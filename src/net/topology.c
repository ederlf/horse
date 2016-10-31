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

struct topology* new_topology(void){
    struct topology* topo = malloc(sizeof(struct topology));
    topo->dps = NULL;
    topo->ndatapaths = 0;
    return topo;
}

void 
add_switch(struct topology *topo, struct datapath* dp){
    HASH_ADD(hh, topo->dps, dp_id, sizeof(uint64_t), dp);
}

void 
destroy_topology(struct topology* topo){
    struct datapath *current_dp, *tmp;
    HASH_ITER(hh, topo->dps, current_dp, tmp) {
        HASH_DEL(topo->dps, current_dp);  
        free(current_dp);
    }
    free(topo);
}
