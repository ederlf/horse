#include "topology.h"

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
