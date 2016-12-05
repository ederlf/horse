#include "event.h"

void 
init_event(struct event *ev, uint8_t type, uint64_t time, uint64_t id){
    ev->type =  type;
    ev->time = time;
    ev->id = id;
}


