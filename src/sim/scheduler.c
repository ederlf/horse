#include "scheduler.h"

struct scheduler* 
scheduler_create(struct topology* topo)
{
    struct scheduler* sch = xmalloc(sizeof(struct scheduler));
    sch->topo = topo;
    sch->clock = 0;
    sch->ev_queue = xmalloc(sizeof(struct heap));
    heap_init(sch->ev_queue);
    return sch;
}

void 
scheduler_insert(struct scheduler *sch, struct event *ev, uint64_t time)
{
    heap_insert(sch->ev_queue, (struct heap_node*) ev, time);
}

void 
scheduler_dispatch(struct scheduler *sch)
{
    struct event *ev = (struct event*) heap_delete(sch->ev_queue);
    sch->clock = ev->time;
    
};

void scheduler_destroy(struct scheduler *sch)
{
    heap_destroy(sch->ev_queue);
    free(sch->ev_queue);
    free(sch);
};