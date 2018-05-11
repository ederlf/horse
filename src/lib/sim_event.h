/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef EVENT_H
#define EVENT_H 1

#include "lib/util.h"
#include "lib/heap.h"
#include "lib/flow.h"
#include "lib/netflow.h"

#define EVENTS_NUM 6
#define FTI_EVENTS_NUM 4

enum events {
    EVENT_FLOW_RECV = 0,
    EVENT_FLOW_SEND = 1, 
    EVENT_PACKET = 2,          /* For the future case of hybrid simulation. */
    EVENT_PORT = 3,            /* Port changes */
    EVENT_FTI = 4,             /* Events that change or keep mode to FTI */
    EVENT_APP_START = 5,       /* Event to mark the start of an app */
    EVENT_END = UINT8_MAX,     /* Final event for the simulation */
};

 /* Events that trigger or maintain Fixed Time Increment Mode */ 
enum fti_events {
    EVENT_OF_MSG_IN = 0,       /* OF message from controller to simulator */
    EVENT_OF_MSG_OUT = 1,      /* Send OF message to controller */
    EVENT_ROUTER_IN = 2,      /* Routing message from emulated part. */
    EVENT_ROUTER_OUT = 3,     /* Routing message to emulated part */
};

/*  The initial field of event heap node so 
*   it can be stored into the simulator's
*   event queue. 
*
*   The priority field of the heap node is 
*   the same as the time value of the event. */

struct sim_event {
    struct heap_node node;      /* ev_node.priority and ev_node.idx .    */
    uint8_t type;               /* Type of the event                     */
    uint64_t time;              /* Time of the event.                    */
    struct sim_event *prev;
    struct sim_event *next;
};

/*  A flow event represents 
*   traffic arriving in a switch.
*/
struct sim_event_flow_recv {
    struct sim_event hdr;       
    uint64_t node_id;           /* The node to process the event.     */
    uint64_t flow_id;
    uint32_t rate;              /* Rate of flow arrived */
    struct netflow *flow;       /* Pointer to the flow, it 
                                   can be modified by the state*/
};

/*  A flow event represents 
*   traffic arriving in a switch.
*/
struct sim_event_flow_send {
    struct sim_event hdr;       
    uint64_t node_id;           /* The node to process the event.     */
    uint64_t flow_id;           /* Retrieve flow */
    uint32_t out_port;
    struct netflow *flow;       /* Pointer to the flow, it 
                                   can be modified by the state*/
};

struct sim_event_fti {
    struct sim_event hdr;
    uint8_t subtype;
    uint8_t *data;  
    size_t len; 
};

/* Start of an application */
struct sim_event_app_start {
    struct sim_event hdr;
    uint64_t node_id;
    struct exec *exec;
};

/* Change to port configuration and/or status */
struct event_port {
    struct sim_event hdr;    
    uint64_t node_id;
    uint8_t config;
    uint8_t status;
};

/* Used for events of OpenFlow Messages */
struct fti_event_of {
    struct sim_event_fti base;       
    uint64_t dp_id;        
};

/* For events between emulated/simulated routers */
struct fti_event_router {
    struct sim_event_fti base;       
    uint32_t router_id; 
};

struct sim_event* sim_event_new(uint64_t time);
void sim_event_free(struct sim_event* ev);
 
struct sim_event_flow_recv *sim_event_flow_recv_new(uint64_t time, 
                                               uint64_t node_id, uint32_t rate); 

struct sim_event_flow_send *sim_event_flow_send_new(uint64_t time, 
                                                    uint64_t node_id, 
                                                    uint32_t out_port);

struct sim_event_fti *sim_event_of_msg_in_new(uint64_t time, 
                                             uint64_t dp_id, void *data, 
                                             size_t len);
struct sim_event_fti *sim_event_of_msg_out_new(uint64_t time, 
                                              uint64_t dp_id, void *data, 
                                              size_t len);
struct sim_event_app_start *sim_event_app_start_new(uint64_t time, uint64_t 
                                              node_id, struct exec *exec);

#endif