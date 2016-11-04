#include "sim.h"

void start(struct topology *topo){
    struct scheduler* sch = scheduler_create(topo);
    scheduler_destroy(sch);
    topology_destroy(topo);
}