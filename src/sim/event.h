#ifndef EVENT_H
#define EVENT_H 1

#include "lib/util.h"
#include "lib/heap.h"
#include "lib/flow.h"

enum events {
    EVENT_FLOW = 0, 
    EVENT_PACKET = 1,     /* For the future case of hybrid simulation. */
    EVENT_CONTROLLER = 2, /* Instructions from the control plane.      */
    EVENT_LINK_DOWN = 3,
    EVENT_LINK_UP = 4,
};

/*  The initial field of events is a
*   pointer to a heap node which so 
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

/*  A flow event represents 
*   traffic arriving in a switch.
*/
struct event_flow{
    uint64_t id;                /* Event key.                           */ 
    uint64_t dpid;              /* The switch to process the event.     */
    uint64_t pkt_cnt;           /* Number of packets in the flow.       */
    uint64_t byte_cnt;          /* Total number of packets in the flow. */ 
    struct flow_key match;      /* The fields belonging to a flow.      */
    UT_hash_handle hh;          /* Make the struct hashable.            */
};

void init_event(struct event *ev, uint8_t type, uint64_t time, uint64_t id);

#endif