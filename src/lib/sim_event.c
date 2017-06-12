/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "sim_event.h"

struct sim_event*
sim_event_new(uint64_t time)
{
    struct sim_event *ev = xmalloc(sizeof(struct sim_event));
    ev->time = time;
    return ev;
}

void sim_event_free(struct sim_event* ev){
    free(ev);
}

struct sim_event_flow* sim_event_flow_new(uint64_t time, uint64_t node_id)
{
    struct sim_event_flow *flow = xmalloc(sizeof(struct sim_event_flow));
    flow->hdr.time = time;
    flow->hdr.type = EVENT_FLOW;
    flow->node_id = node_id;
    return flow;   
}

