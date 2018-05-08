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

static void 
init_event_hdr(struct sim_event* ev, uint64_t time, uint8_t type) 
{
    ev->time = time;
    ev->type = type;
}

static void 
init_fti_base(struct sim_event_fti *ev, uint16_t subtype, uint8_t *data,
              size_t len)
{
    ev->subtype = subtype;
    ev->data = data;
    ev->len = len;
}

static void 
fti_event_free(struct sim_event_fti *ev)
{
    switch (ev->subtype) {
        case EVENT_OF_MSG_IN:
        case EVENT_OF_MSG_OUT: {
            free((struct fti_event_of*) ev);
            break;
        }
        case EVENT_ROUTER_IN:
        case EVENT_ROUTER_OUT: {
            free((struct fti_event_router*) ev);
            break;   
        }
        default: {
            fprintf(stderr, "Unknown FTI Event %d\n", ev->subtype);
            free(ev);
            break;
        }
    }
}

void sim_event_free(struct sim_event* ev){
    
    switch (ev->type){
        case EVENT_FLOW_RECV:{
            free((struct sim_event_flow_recv*) ev);
            break;
        } 
        case EVENT_FLOW_SEND: {
            /* The pointer fields should be released in 
               the end of their lifetime */
            free((struct sim_event_flow_send*) ev);
            break;
        }
        case EVENT_FTI: {
            fti_event_free((struct sim_event_fti*) ev);
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
            free(ev);
            break;
        }
    } 
}
 
struct sim_event_flow_recv *sim_event_flow_recv_new(uint64_t time,
                                                    uint64_t node_id, 
                                                    uint32_t rate) 
{
    struct sim_event_flow_recv *flow = xmalloc(sizeof(struct 
                                               sim_event_flow_recv));
    init_event_hdr(&flow->hdr, time, EVENT_FLOW_RECV);
    flow->node_id = node_id;
    flow->rate = rate;
    return flow;   
}

struct sim_event_flow_send* 
sim_event_flow_send_new(uint64_t time, uint64_t node_id, uint32_t out_port)
{
    struct sim_event_flow_send *flow = xmalloc(sizeof(struct sim_event_flow_send));
    init_event_hdr(&flow->hdr, time, EVENT_FLOW_SEND);
    flow->node_id = node_id;
    flow->out_port = out_port;
    return flow;  
}

static struct fti_event_of* 
of_msg_new(uint64_t time, uint64_t dp_id, void *data, size_t len, uint8_t type)
{
    struct fti_event_of *msg = xmalloc(sizeof(struct fti_event_of));
    init_event_hdr(&msg->base.hdr, time, EVENT_FTI);
    init_fti_base(&msg->base, type, data, len);
    msg->dp_id = dp_id;
    return msg;        
} 

struct sim_event_fti* 
sim_event_of_msg_in_new(uint64_t time, uint64_t dp_id, void *data, size_t len)
{
    return (struct sim_event_fti*) of_msg_new(time, dp_id, data, len,
                                              EVENT_OF_MSG_IN);      
}

struct sim_event_fti* 
sim_event_of_msg_out_new(uint64_t time, uint64_t dp_id, void *data, size_t len)
{
    return (struct sim_event_fti*) of_msg_new(time, dp_id, data, len,
                                              EVENT_OF_MSG_OUT);      
}

struct sim_event_app_start*
sim_event_app_start_new(uint64_t time, uint64_t node_id, struct exec *exec)
{
    struct sim_event_app_start *ev = xmalloc(sizeof(struct 
                                             sim_event_app_start));
    init_event_hdr(&ev->hdr, time, EVENT_APP_START);
    ev->node_id = node_id;
    ev->exec = exec;
    return ev;
}
