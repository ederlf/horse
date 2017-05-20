/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef SIM_H
#define SIM_H 1

#include "scheduler.h"
#include "lib/redis_ipc.h"
#include "net/topology.h"
#include "lib/timer.h"

enum mode {
    DES = 0,
    CONTINUOUS = 1,
};

struct sim {
    struct topology  *topo;     /* Topology of the simulation. */
    struct scheduler *sch;      /* Event scheduler.            */
    struct redis_ipc *ri;
    bool mode;
};

void start(struct topology *topo);

#endif