#include "sim.h"

static void
sim_init(struct sim *s, struct topology *topo) {
    s->topo = topo;
    s->events = NULL;
    s->sch = scheduler_new();
}

static void 
sim_clean_ev(struct event_hdr *events){
  struct event_hdr *ev, *tmp;
  HASH_ITER(hh, events, ev, tmp) {
    HASH_DEL(events, ev);  
    free(ev);           
  }
}

static void 
sim_close(struct sim *s){
    sim_clean_ev(s->events);
    scheduler_destroy(s->sch);
    topology_destroy(s->topo);
}

void 
start(struct topology *topo) {
    struct sim s;
    sim_init(&s, topo);
    sim_close(&s);    
}

