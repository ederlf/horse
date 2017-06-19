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

#include "event_handler.h"
#include "net/topology.h"
#include "lib/timer.h"


enum sim_mode {
    SIM_CTRL = 0,
    EMU_CTRL = 1,
};

struct sim {
    struct ev_handler evh;
    struct timer cont;          /* Timer for continuous mode   */
    pthread_t dataplane;
    enum sim_mode mode;
    union {
        struct of_manager *om; /* Real controllers */
        //struct controller *ctrl; /* Simulated control */
    };
};

void start(struct topology *topo);

#endif