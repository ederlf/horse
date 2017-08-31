/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "sim.h"
#include <unistd.h>
#include <time.h>
#include <uthash/utlist.h>
#include <valgrind/memcheck.h>

#define EV_NUM 1000000


static void *des_mode(void * args);
static void *cont_mode(void* args); 

static pthread_cond_t  mode_cond_var   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx_mode    = PTHREAD_MUTEX_INITIALIZER;
int last_wrt = 0;
uint64_t last_ctrl = 0;

static void 
initial_events(struct sim *s)
{
    struct scheduler *sch = s->evh.sch;
    struct topology *topo = s->evh.topo;
    struct node *node, *tmp;
    /* Not very efficient now... */
    HASH_ITER(hh, topology_nodes(topo), node, tmp){
        if (node->type == HOST){
            struct exec *exec, *exec_tmp;
            HASH_ITER(hh, host_execs((struct host*) node), exec, exec_tmp) {
                struct sim_event_app_start *ev = sim_event_app_start_new(exec->start_time, node->uuid, exec);
                scheduler_insert(sch, (struct sim_event*) ev);
            }   
        }
    }
    /* Event to stop the simulator */
    struct sim_event *end_ev; 
    end_ev = sim_event_new(UINT64_MAX); 
    end_ev->type = EVENT_END;
    scheduler_insert(sch, end_ev);
}

struct timespec last = {0};
struct timespec now = {0};

static void
sim_init(struct sim *s, struct topology *topo, enum sim_mode mode) 
{
    s->evh.topo = topo;
    s->evh.sch = scheduler_new();
    s->cont.exec = cont_mode;
    s->mode = mode;
    initial_events(s); 
    if (s->mode == EMU_CTRL){
        struct node *cur_node, *tmp, *nodes;
        struct datapath *dp;
        s->evh.om = of_manager_new(s->evh.sch);
        /* Add of_settings to client */
        nodes = topology_nodes(topo);
        HASH_ITER(hh, nodes, cur_node, tmp) {
            if (cur_node->type == DATAPATH){
                dp = (struct datapath*) cur_node;
                of_client_add_ofsc(s->evh.om->of, dp_settings(dp));
            }
        }
        of_client_start(s->evh.om->of, false);
    }
    init_timer(&s->cont, (void*)s);
    set_periodic_timer(/* 1 */100);
    clock_gettime(CLOCK_MONOTONIC_RAW, &last);
    pthread_create(&s->dataplane, (pthread_attr_t*)0, des_mode, (void*)s);
}

static void 
sim_close(struct sim *s)
{
    pthread_join(s->dataplane, 0);
    scheduler_destroy(s->evh.sch);
    topology_destroy(s->evh.topo);
    of_manager_destroy(s->evh.om);
}


// static void update_stats(struct topology *topo, uint64_t time){

//     struct node *cur_node, *tmp, *nodes;
//     nodes = topology_nodes(topo);
//     HASH_ITER(hh, nodes, cur_node, tmp) {
//         if (cur_node->type == DATAPATH){
//             dp_write_stats((struct datapath*) cur_node, time);
//         }
//     }
// }

static void *
des_mode(void *args){
/*  Executes DES while controller does nothing
    Waits for some event from controller or current time
    is larger than event time */
    struct sim *s = (struct sim*) args;
    struct scheduler *sch = s->evh.sch;   
    while (1){
        // printf("DP %d %d %d\n", cur_time, events[cur_ev].tm, cur_ev);
        /* Only execute in DES mode */
        pthread_mutex_lock( &mtx_mode );
        while(sch->mode){
            // printf("Waiting\n");
            pthread_cond_wait( &mode_cond_var, &mtx_mode );
        }
        pthread_mutex_unlock( &mtx_mode );

        struct sim_event *ev = scheduler_dispatch(sch);
        sch->clock = ev->time;

        /* Time will be shared now */
        if (sch->clock - last_wrt > 1000000){
            // update_stats(s->topo, s->sch->clock);
            last_wrt = sch->clock;
        }
        /* Need to make it more sane ... */
        if (ev->type != EVENT_END) {
            /* Execute */
            handle_event(&s->evh, ev);
            if (ev->type == EVENT_OF_MSG_OUT ||
                ev->type == EVENT_OF_MSG_IN ) {
                
                /* Wake up timer */
                pthread_mutex_lock( &mtx_mode );
                last_ctrl = sch->clock;
                sch->mode = CONTINUOUS;
                pthread_cond_signal( &mode_cond_var );
                pthread_mutex_unlock( &mtx_mode );
            
            }
            sim_event_free(ev);
        }
        else {
            printf("End\n");
            sim_event_free(ev);
            break;
        }
        
    }
    printf("Over timer %p\n", s);
    shutdown_timer(&s->cont);
    return 0;
} 

struct sim_event *cur_ev = NULL;
uint64_t mode_interval = 100000; // in microseconds 

uint64_t delta_us = 0;

static void *
cont_mode(void* args) 
{

    struct sim *s = (struct sim*) args;
    struct scheduler *sch = s->evh.sch;
    /* The code below is just a demonstration. */
    /* Increase time and check if there a DP event to execute */
    pthread_mutex_lock( &mtx_mode );
    while(!sch->mode){
        pthread_cond_wait( &mode_cond_var, &mtx_mode );
    }
    pthread_mutex_unlock( &mtx_mode );
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    uint64_t delta_us = (now.tv_sec - last.tv_sec) * 1000000 + (now.tv_nsec - last.tv_nsec) / 1000;
    cur_ev = scheduler_retrieve(sch); 
    sch->clock += delta_us;     
    if (cur_ev->time <= sch->clock){
        /* Execute */
        if (cur_ev->type == EVENT_OF_MSG_OUT || 
             cur_ev->type == EVENT_OF_MSG_IN ) {
            last_ctrl = cur_ev->time;
        }
        else if(cur_ev->type == EVENT_END){
            printf("Last event?\n");
            goto check_idle;
        }
        handle_event(&s->evh, cur_ev);
        scheduler_delete(sch, cur_ev);
        sim_event_free(cur_ev);
        cur_ev = NULL;
    }
            
    check_idle:
    clock_gettime(CLOCK_MONOTONIC_RAW, &last);
    /* Check if controller is idle for some time */
    if (sch->clock - last_ctrl > mode_interval){
        pthread_mutex_lock( &mtx_mode );
        sch->mode = DES;
        /* Wake up timer */
        pthread_cond_signal( &mode_cond_var );
        pthread_mutex_unlock( &mtx_mode );
    }
    return 0; 
}

void 
start(struct topology *topo) 
{
    struct sim s;
    memset(&s, 0x0, sizeof(struct sim));
    sim_init(&s, topo, EMU_CTRL);
    sim_close(&s);    
}

