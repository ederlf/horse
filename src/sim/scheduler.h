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
#include <pthread.h>
#include <uthash/utlist.h>

enum scheduler_mode {
    DES = 0, /* Traditional Discrete Event Operation */
    FTI = 1  /* Fixed Time increment, time advances in equal intervals */
};

struct scheduler {
    uint64_t clock;          /* Current time of the simulation. */
    // struct heap *ev_queue;   /* Scheduled events. */
    struct sim_event *ev_queue;
    pthread_mutex_t sch_mutex;                
    enum scheduler_mode mode;
};

struct scheduler *scheduler_new(void);
bool scheduler_is_empty(struct scheduler *sch);
void scheduler_destroy(struct scheduler *sch);
void scheduler_insert(struct scheduler *sch, struct sim_event *ev);
void scheduler_delete(struct scheduler *sch, struct sim_event *ev);
struct sim_event *scheduler_retrieve(struct scheduler *sch);
struct sim_event *scheduler_dispatch(struct scheduler *sch);


#endif