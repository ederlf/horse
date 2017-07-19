/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */
 
#include "scheduler.h"

struct scheduler* 
scheduler_new()
{
    struct scheduler *sch = xmalloc(sizeof(struct scheduler));
    sch->clock = 0; /* Use time == now()? */
    sch->mode = CONTINUOUS; /* Scheduler always starts like CONTINUOUS */
    sch->ev_queue = xmalloc(sizeof(struct heap));
    heap_init(sch->ev_queue);
    pthread_mutex_init(&sch->sch_mutex, NULL);
    return sch;
}

/* If size == 0, scheduler is empty */
bool 
scheduler_is_empty(struct scheduler *sch)
{
    return sch->ev_queue->size? false: true;
}

void 
scheduler_destroy(struct scheduler *sch)
{
    heap_destroy(sch->ev_queue);
    free(sch->ev_queue);
    free(sch);
};

void 
scheduler_insert(struct scheduler *sch, struct sim_event *ev)
{
    heap_insert(sch->ev_queue, (struct heap_node*) ev, ev->time);
}


struct sim_event*
scheduler_dispatch(struct scheduler *sch)
{
    struct sim_event *ev = (struct sim_event*) heap_delete(sch->ev_queue);
    // sch->clock = ev->time;
    return ev;
};
