#ifndef EVENT_H
#define EVENT_H 1

#include "lib/util.h"
#include "lib/heap.h"

/*  The initial field of events is a
*   pointer to a heap node which so 
*   it can be stored into the simulator's
*   event queue. 
*
*   The priority field of the heap node is 
*   the same as the time value of the event. 
*/
struct event {
    struct heap_node *ev_node; /* ev_node.priority and ev_node.idx . */
    uint8_t type;              /* Type of the event to handle.      */
    uint64_t time;              /* Time of the event */
};

struct event_dummy {
    struct event ev_hdr;
    char msg[10];
};

#endif