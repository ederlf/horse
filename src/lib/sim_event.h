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

#define EVENTS_NUM 8

enum events {
    EVENT_FLOW_RECV = 0,
    EVENT_FLOW_SEND = 1, 
    EVENT_PACKET = 2,          /* For the future case of hybrid simulation. */
    EVENT_PORT = 3,            /* Port changes */
    EVENT_OF_MSG_IN = 4,       /* OF message from controller to simulator */
    EVENT_OF_MSG_OUT = 5,      /* Send OF message to controller */
    EVENT_APP_START = 6,       /* Event to mark the start of an app */
    EVENT_END = UINT8_MAX,     /* Final event for the simulation */
};

/*  The initial field of event heap node so 
*   it can be stored into the simulator's
*   event queue. 
*
*   The priority field of the heap node is 
*   the same as the time value of the event. 
*/
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
    uint64_t src_id;            /* Id of the sender */
    uint32_t src_port;          /* Port of the sender */
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
    uint32_t out_port;
    struct netflow *flow;       /* Pointer to the flow, it 
                                   can be modified by the state*/
};

/* Used for events of OpenFlow Messages */
struct sim_event_of {
    struct sim_event hdr;       
    uint64_t dp_id;
    uint8_t *data;  
    size_t len;        
};

/* Start of an application */
struct sim_event_app_start {
    struct sim_event hdr;
    uint64_t node_id;
    struct exec *exec;
};

/* Flow event when a packet/flow is sent to the controller 
   It does not have a node_id because it is not handled by a node */
struct sim_event_pkt_in {
    struct sim_event hdr;       
    uint64_t dp_id;         /* The switch that generated the event.     */
    uint32_t buffer_id;     /* ID assigned by datapath. */
    uint16_t total_len;     /* Full length of frame. */
    uint8_t reason;         /* Reason packet is being sent (one of OFPR_*) */
    uint8_t table_id;       /* ID of the table that was looked up */
    uint64_t cookie;        /* Cookie of the flow entry that was looked up. */
    struct netflow flow;
};

struct sim_event_pkt_out {
    struct sim_event hdr; 
    uint64_t dp_id;
    uint32_t buffer_id;           /* ID assigned by datapath (OFP_NO_BUFFER
                                     if none). */
    uint32_t in_port;             /* Packet's input port  */
    struct netflow flow;
};

/* Change to port configuration and/or status */
struct event_port {
    struct sim_event hdr;    
    uint64_t node_id;
    uint8_t config;
    uint8_t status;
};

struct sim_event* sim_event_new(uint64_t time);
void sim_event_free(struct sim_event* ev);
 
struct sim_event_flow_recv *sim_event_flow_recv_new(uint64_t time, 
                                               uint64_t node_id, uint64_t src_id, 
                                               uint32_t src_port, uint32_t rate); 

struct sim_event_flow_send *sim_event_flow_send_new(uint64_t time, 
                                                    uint64_t node_id, 
                                                    uint32_t out_port);

struct sim_event_of *sim_event_of_msg_in_new(uint64_t time, 
                                             uint64_t dp_id, void *data, 
                                             size_t len);
struct sim_event_of *sim_event_of_msg_out_new(uint64_t time, 
                                              uint64_t dp_id, void *data, 
                                              size_t len);
struct sim_event_app_start *sim_event_app_start_new(uint64_t time, uint64_t 
                                              node_id, struct exec *exec);

#endif