#include "scheduler.h"

struct scheduler* 
scheduler_new()
{
    struct scheduler* sch = xmalloc(sizeof(struct scheduler));
    sch->clock = 0; /* Use time == now()? */
    sch->ev_queue = xmalloc(sizeof(struct heap));
    heap_init(sch->ev_queue);
    return sch;
}

void 
scheduler_destroy(struct scheduler *sch)
{
    heap_destroy(sch->ev_queue);
    free(sch->ev_queue);
    free(sch);
};

void 
scheduler_insert(struct scheduler *sch, struct event* ev)
{
    /* TODO: Return error if event time is smaller than clock */
    if (ev->time < sch->clock){
        return;
    }
    heap_insert(sch->ev_queue, (struct heap_node*) ev, ev->time);
}


struct event
scheduler_dispatch(struct scheduler *sch)
{
    struct event *ev = (struct event*) heap_delete(sch->ev_queue);
    sch->clock = ev->time;
    return *ev;
};
