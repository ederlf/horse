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

enum events {
    EVENT_FLOW = 0, 
    EVENT_PACKET = 1,     /* For the future case of hybrid simulation. */
    EVENT_INSTRUCTION = 2, /* Instructions from the control plane.      */
    EVENT_PORT = 3,
};

/*  The initial field of event heap node so 
*   it can be stored into the simulator's
*   event queue. 
*
*   The priority field of the heap node is 
*   the same as the time value of the event. 
*/
struct event {
    struct heap_node node;      /* ev_node.priority and ev_node.idx .    */
    uint8_t type;               /* Type of the event to handle.          */
    uint64_t time;              /* Time of the event.                    */
    uint64_t id;                /* Retrieve it in hash table of events.  */  
};

/* Common header for all events */
struct event_hdr {
    UT_hash_handle hh;          /* Make the struct hashable.            */
    uint64_t id;                /* Event key.                           */ 
    uint8_t type;               /* Type of the event.                   */
};

/*  A flow event represents 
*   traffic arriving in a switch.
*/
struct event_flow {
    struct event_hdr hdr;       
    uint64_t dpid;              /* The switch to process the event.     */
    uint64_t pkt_cnt;           /* Number of packets in the flow.       */
    uint64_t byte_cnt;          /* Total number of packets in the flow. */ 
    struct flow_key match;      /* The fields belonging to a flow.      */
};

/* A instruction from the control plane. */
struct event_instruction {
    struct event_hdr hdr;       
    uint64_t dpid;              /* The switch to process the event.     */
    //struct instruction

};

/* Change to port configuration and/or status */
struct event_port {
    struct event_hdr hdr;    
    uint64_t dpid;
    uint8_t config;
    uint8_t status;
};

void init_event(struct event *ev, uint8_t type, uint64_t time, uint64_t id);

#endif