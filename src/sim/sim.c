#include "sim.h"

void start(struct topology *topo){
    struct scheduler* sch = scheduler_new();
    scheduler_destroy(sch);
    topology_destroy(topo);
}