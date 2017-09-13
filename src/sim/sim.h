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

#include "sim_config.h"
#include "event_handler.h"
#include "net/topology.h"
#include "lib/timer.h"

struct sim {
    struct ev_handler evh;
    struct timer cont;          /* Timer for continuous mode   */
    pthread_t dataplane;
    struct sim_config *config;
};

void start(struct topology *topo, struct sim_config *config);

#endif