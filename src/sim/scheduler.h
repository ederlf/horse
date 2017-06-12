/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H 1

#include "lib/sim_event.h"
#include "lib/heap.h"

struct scheduler {
    uint64_t clock;          /* Current time of the simulation. */
    struct heap *ev_queue;   /* Scheduled events.               */  
};

struct scheduler *scheduler_new(void);
bool scheduler_is_empty(struct scheduler *sch);
void scheduler_destroy(struct scheduler *sch);
void scheduler_insert(struct scheduler *sch, struct sim_event *ev);
struct sim_event *scheduler_dispatch(struct scheduler *sch);


#endif