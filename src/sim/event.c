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


void 
init_event(struct event *ev, uint8_t type, uint64_t time, uint64_t id){
    ev->type =  type;
    ev->time = time;
    ev->id = id;
}


