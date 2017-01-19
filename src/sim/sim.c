/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "sim.h"
#include "event_handler.h"

#define EV_NUM 10

static void 
create_random_events(struct sim *s, struct event *ev)
{
	int i;
	for (i = 0; i < EV_NUM; ++i){
		struct event_flow *flow = malloc(sizeof(struct event_flow));
	    flow->hdr.id = i + 1;
	    flow->hdr.type = EVENT_FLOW;
	    flow->dpid = 0x0000000000000001;
	    HASH_ADD(hh, s->events, id, sizeof(uint64_t), (struct event_hdr*) flow);
	    init_event(&ev[i], EVENT_FLOW, i + 1, i + 1); 
	    scheduler_insert(s->sch, &ev[i]);
	}
}

static void
sim_init(struct sim *s, struct topology topo) 
{
    s->topo = topo;
    s->events = NULL;
    s->sch = scheduler_new();
}

static void 
sim_clean_ev(struct event_hdr *events)
{
  struct event_hdr *ev, *tmp;
  HASH_ITER(hh, events, ev, tmp) {
      HASH_DEL(events, ev);  
      free(ev);           
  }
}

static void 
sim_close(struct sim *s)
{
    sim_clean_ev(s->events);
    scheduler_destroy(s->sch);
    topology_destroy(&s->topo);
}

static void
sim_execute_event(struct sim *s)
{
	struct event_hdr *ev;
	struct event sch_ev = scheduler_dispatch(s->sch);
	/* Event MUST exist in the hash */
	HASH_FIND(hh, s->events, &sch_ev.id, sizeof(uint64_t), ev);
	handle_event(&s->topo, ev);

}

void 
start(struct topology topo) 
{
    struct sim s;
    struct event* ev = malloc(sizeof(struct event) * EV_NUM);
    sim_init(&s, topo);
    create_random_events(&s, ev);
    while (!scheduler_is_empty(s.sch)) {
    	sim_execute_event(&s);
    }
    sim_close(&s);    
}

