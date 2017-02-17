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
create_random_events(struct sim *s)
{
	int i;
	for (i = 0; i < EV_NUM; ++i){
        struct event *ev = malloc(sizeof(struct event));
    	struct event_flow *flow = malloc(sizeof(struct event_flow));
        flow->hdr.id = i + 1;
        flow->hdr.time = i + 1;
        flow->hdr.type = EVENT_FLOW;
        flow->node_id = 1;
        flow->flow.start_time = flow->hdr.time;
        flow->flow.end_time = flow->flow.start_time + 1;
        memset(&flow->flow.match, 0x0, sizeof(struct flow_key));
        flow->flow.match.in_port = 1;
        HASH_ADD(hh, s->events, id, sizeof(uint64_t), 
                 (struct event_hdr*) flow);
        init_event(ev, flow->hdr.time , flow->hdr.id); 
        scheduler_insert(s->sch, ev);
	}
}

static void add_flows(struct topology *topo){
    /* DP 1 */
    struct datapath *dp = (struct datapath*) topology_node(topo, 1);
    struct flow *fl = flow_new();
    struct instruction_set is;
    instruction_set_init(&is);
    struct write_metadata wm;
    inst_write_metadata(&wm, 0xbeef);
    add_write_metadata(&is, wm);
    /* Write actions */
    struct write_actions wa;
    struct action_set as;
    struct action gen_act;
    action_set_init(&as);
    action_output(&gen_act, 2);
    action_set_add(&as, gen_act);
    action_set_field_u16(&gen_act, SET_IP_PROTO, 6);
    action_set_add(&as, gen_act);
    inst_write_actions(&wa, as);
    add_write_actions(&is, wa);
    /* Match */
    set_in_port(fl, 1);
    flow_add_instructions(fl, is);
    dp_handle_flow_mod(dp, 0, fl, 0);
    
    /* DP 2 */
    dp = (struct datapath*) topology_node(topo, 2);
    struct flow *fl2 = flow_new();
    instruction_set_init(&is);
    struct apply_actions aa;
    struct action_list al;
    action_list_init(&al);
    action_output(&gen_act, 2);
    action_list_add(&al, gen_act);
    inst_apply_actions(&aa, al);
    add_apply_actions(&is, aa);
    /* Match */
    set_in_port(fl2, 2);
    set_eth_type(fl2, 0x800);
    flow_add_instructions(fl2, is);
    dp_handle_flow_mod(dp, 0, fl2, 0);
}

static void
sim_init(struct sim *s, struct topology *topo) 
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
    topology_destroy(s->topo);
}

static void
sim_execute_event(struct sim *s)
{
	struct event_hdr *ev;
	struct event *sch_ev = scheduler_dispatch(s->sch);
	/* Event MUST exist in the hash */
	HASH_FIND(hh, s->events, &sch_ev->id, sizeof(uint64_t), ev);
	handle_event(s->sch, s->topo, s->events, ev);
    free(sch_ev);
}

void 
start(struct topology *topo) 
{
    struct sim s;
    sim_init(&s, topo);
    add_flows(topo);
    create_random_events(&s);
    while (!scheduler_is_empty(s.sch)) {
    	sim_execute_event(&s);
    }
    sim_close(&s);    
}

