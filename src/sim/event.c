/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "event.h"

static uint64_t event_id = 0;

struct event*
event_new(uint64_t time, uint64_t id)
{
    struct event *ev = xmalloc(sizeof(struct event));
    ev->time = time;
    ev->id = id;
    return ev;
}


void event_free(struct event* ev){
    free(ev);
}

struct event_flow* event_flow_new(uint64_t time, uint64_t node_id)
{
    struct event_flow *flow = xmalloc(sizeof(struct event_flow));
    flow->hdr.id = ++event_id;
    flow->hdr.time = time;
    flow->hdr.type = EVENT_FLOW;
    flow->node_id = node_id;
    return flow;   
}

