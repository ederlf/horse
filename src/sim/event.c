/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "event.h"

struct event*
event_new(uint64_t time, uint64_t id)
{
    struct event *ev = xmalloc(sizeof(struct event));
    ev->time = time;
    ev->id = id;
    return ev;
}


void event_free(struct event* ev){
    free(ev);
}
