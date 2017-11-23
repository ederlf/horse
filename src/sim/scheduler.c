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

static 
int priocmp(struct sim_event *a, struct sim_event *b) {
    // printf("Comparing\n");
    if (a->time < b->time)
        return -1;  // Return -1 if you want ascending, 1 if you want descending order. 
    else if (a->time > b->time)
        return 1;   // Return 1 if you want ascending, -1 if you want descending order. 
    return 0;
}


struct scheduler* 
scheduler_new()
{
    struct scheduler *sch = xmalloc(sizeof(struct scheduler));
    sch->clock = 0; /* Use time == now()? */
    sch->mode = CONTINUOUS; /* Scheduler always starts like CONTINUOUS */
    // sch->ev_queue = xmalloc(sizeof(struct heap));
    // heap_init(sch->ev_queue);
    sch->ev_queue = NULL;
    pthread_mutex_init(&sch->sch_mutex, NULL);
    return sch;
}

/* If size == 0, scheduler is empty */
bool 
scheduler_is_empty(struct scheduler *sch)
{
    // return sch->ev_queue->size? false: true;
    return sch->ev_queue == NULL? true:false;
}

void 
scheduler_destroy(struct scheduler *sch)
{
    // heap_destroy(sch->ev_queue);
    // free(sch->ev_queue);
    free(sch);
};

void 
scheduler_insert(struct scheduler *sch, struct sim_event *ev)
{
    pthread_mutex_lock(&sch->sch_mutex);
    // heap_insert(sch->ev_queue, (struct heap_node*) ev, ev->time);
    // DL_APPEND(sch->ev_queue, ev);
    // DL_SORT(sch->ev_queue, ev)
    LL_INSERT_INORDER(sch->ev_queue, ev, priocmp);
    pthread_mutex_unlock(&sch->sch_mutex);
}

/* Just delete the event on top without returning it */
void
scheduler_delete(struct scheduler *sch, struct sim_event *ev)
{
    pthread_mutex_lock(&sch->sch_mutex);
    // heap_delete(sch->ev_queue);
    LL_DELETE(sch->ev_queue, ev);
    pthread_mutex_unlock(&sch->sch_mutex);
}

/* Get the first event in the queue */
struct sim_event *
scheduler_retrieve(struct scheduler *sch)
{   
    struct sim_event *ev = sch->ev_queue;
    // ev = (struct sim_event*) heap_retrieve(sch->ev_queue);
    // 
    return ev;
}

struct sim_event*
scheduler_dispatch(struct scheduler *sch)
{
    pthread_mutex_lock(&sch->sch_mutex);
    // struct sim_event *ev = (struct sim_event*) heap_delete(sch->ev_queue);
    struct sim_event *ev = NULL;
    ev = sch->ev_queue;
    // printf("PQP %p %p %p %d\n", sch->ev_queue->next, sch->ev_queue, sch->ev_queue->prev, ev->type ); 
    LL_DELETE(sch->ev_queue, sch->ev_queue);
    pthread_mutex_unlock(&sch->sch_mutex);
    // sch->clock = ev->time;
    return ev;
};
