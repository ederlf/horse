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

enum events {
    EVENT_FLOW = 0, 
    EVENT_PACKET = 1,     /* For the future case of hybrid simulation. */
    EVENT_INSTRUCTION = 2, /* Instructions from the control plane.      */
    EVENT_PORT = 3,
    EVENT_CTRL = 4,
    EVENT_END = UINT8_MAX, /* Final event for the simulation */
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
};

/*  A flow event represents 
*   traffic arriving in a switch.
*/
struct sim_event_flow {
    struct sim_event hdr;       
    uint64_t node_id;           /* The switch to process the event.     */
    struct netflow flow;
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
    uint64_t node_id;
    uint32_t buffer_id;           /* ID assigned by datapath (OFP_NO_BUFFER
                                     if none). */
    uint32_t in_port;             /* Packet's input port  */
    struct netflow flow;
};

/* A instruction from the control plane. */
struct event_instruction {
    struct sim_event hdr;       
    uint64_t node_id;              /* The node to process the event.     */
    //struct instruction
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
struct sim_event_flow* sim_event_flow_new(uint64_t time, uint64_t node_id);

#endif