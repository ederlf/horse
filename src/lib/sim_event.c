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
    
    switch (ev->type){
        case EVENT_FLOW: {
            free((struct sim_event_flow*) ev);
            break;
        }
        case EVENT_OF_MSG_IN:
        case EVENT_OF_MSG_OUT: {
            free((struct sim_event_of*) ev);
            break;
        }
        case EVENT_APP_START: {
            free((struct sim_event_app_start*) ev);
            break;
        }
        case EVENT_END :{
            free(ev);
            break;
        }
        default: {
            fprintf(stderr, "Unknown Event %d\n", ev->type);
            break;
        }
    } 
}
 
struct sim_event_flow*
sim_event_flow_new(uint64_t time, uint64_t node_id)
{
    struct sim_event_flow *flow = xmalloc(sizeof(struct sim_event_flow));
    flow->hdr.time = time;
    flow->hdr.type = EVENT_FLOW;
    flow->node_id = node_id;
    return flow;   
}

static struct sim_event_of* 
of_msg_new(uint64_t time, uint64_t dp_id, void *data, 
           size_t len, uint8_t type)
{
    struct sim_event_of *msg = xmalloc(sizeof(struct sim_event_of));
    msg->hdr.time = time;
    msg->hdr.type = type;
    msg->dp_id = dp_id;
    msg->data = data;
    msg->len = len;
    return msg;        
} 

struct sim_event_of* 
sim_event_of_msg_in_new(uint64_t time, uint64_t dp_id, 
                         void *data, size_t len)
{
    return of_msg_new(time, dp_id, data, len, EVENT_OF_MSG_IN);      
}

struct sim_event_of* 
sim_event_of_msg_out_new(uint64_t time, uint64_t dp_id, 
                         void *data, size_t len)
{
    return of_msg_new(time, dp_id, data, len, EVENT_OF_MSG_OUT);      
}

struct sim_event_app_start*
sim_event_app_start_new(uint64_t time, uint64_t node_id, struct exec *exec)
{
    struct sim_event_app_start *ev = xmalloc(sizeof(
                                             struct sim_event_app_start));
    ev->hdr.time = time;
    ev->hdr.type = EVENT_APP_START;
    ev->node_id = node_id;
    ev->exec = exec;
    return ev;
}
