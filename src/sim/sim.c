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

#define EV_NUM 1000000

enum tests {
    SINGLE = 0,
    LINEAR = 1,
    CORE_EDGE = 2,
};

static void *des_mode(void * args);
static void cont_mode(void* args); 

static void 
initial_events(struct sim *s)
{
    struct scheduler *sch = s->evh.sch;
    struct topology *topo = s->evh.topo;
    struct node *node, *tmp;
    /* Not very efficient now... */
    HASH_ITER(hh, topology_nodes(topo), node, tmp){
        struct exec *exec, *exec_tmp;
        if (node->type == HOST){
            HASH_ITER(hh, host_execs((struct host*) node), exec, exec_tmp) {
                struct sim_event_app_start *ev = sim_event_app_start_new(exec->start_time, node->uuid, exec);
                scheduler_insert(sch, (struct sim_event*) ev);
            }   
        }
    }
    /* Event to stop the simulator */
    struct sim_event *ev; 
    ev = sim_event_new(UINT64_MAX - 1); 
            scheduler_insert(sch, ev);
    ev->type = EVENT_END;
    scheduler_insert(sch, ev);
}

static void
sim_init(struct sim *s, struct topology *topo, enum sim_mode mode) 
{
    s->evh.topo = topo;
    s->evh.sch = scheduler_new();
    s->cont.exec = cont_mode;
    // init_timer(s->cont, (void*)s);
    // set_periodic_timer(/* 1 */100);
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
        // printf("Started connections %p\n", des_mode);
    }
    init_timer(s->cont, (void*)s);
    set_periodic_timer(/* 1 */10);
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

static pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex1    = PTHREAD_MUTEX_INITIALIZER;
int run = 0;
int last_wrt = 0;
uint64_t last_ctrl = 0;

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
des_mode(void * args){
/*  Executes DES while controller does nothing
    Waits for some event from controller or current time
    is larger than event time */
    struct sim *s = (struct sim*) args;
    struct scheduler *sch = s->evh.sch;
    while (1){
        // printf("DP %d %d %d\n", cur_time, events[cur_ev].tm, cur_ev);
        /* Only execute in DES mode */
        pthread_mutex_lock( &mutex1 );
        while(sch->mode){
            // printf("Waiting\n");
            pthread_cond_wait( &condition_var, &mutex1 );
        }
        pthread_mutex_unlock( &mutex1 );
        if (scheduler_is_empty(sch)){
            continue;
        }

        struct sim_event *ev = scheduler_dispatch(sch);
        sch->clock = ev->time;

        // printf("Clock DES %"PRIu64"\n", sch->clock);
        /* Time will be shared now */
        if (sch->clock - last_wrt > 1000000){
            // update_stats(s->topo, s->sch->clock);
            last_wrt = sch->clock;
        }
        /* Need to make it more sane ... */
        if (ev->type == EVENT_FLOW || ev->type == EVENT_APP_START) {
            /* Execute */
            handle_event(&s->evh, ev);
            sim_event_free(ev);
        }
        else if( ev->type == EVENT_OF_MSG_OUT ||
                ev->type == EVENT_OF_MSG_IN){
            // printf("MSG_OF DES %s %ld\n", ev->type == 4? "OUT":"IN", 
            //        ev->time);
            handle_event(&s->evh, ev);
            sim_event_free(ev);
            printf("Switching to CONT\n");
            pthread_mutex_lock( &mutex1 );
            last_ctrl = sch->clock;
            sch->mode = CONTINUOUS;
            /* Wake up timer */
            pthread_cond_signal( &condition_var );
            pthread_mutex_unlock( &mutex1 );
        }
        else if (ev->type == EVENT_END) {
            sim_event_free(ev);
            break;
        }
    }
    /* Awake the timer so it can stop */
    // pthread_mutex_lock( &mutex1 );
    printf("Over timer %p\n", s);
    shutdown_timer(s->cont);
    // pthread_cond_signal( &condition_var );
    // pthread_mutex_unlock( &mutex1 );
    return 0;
} 

struct sim_event *cur_ev = NULL;
uint64_t mode_interval = 1000000; // in microseconds 

static void 
cont_mode(void* args) 
{
    struct sim *s = (struct sim*) args;
    struct scheduler *sch = s->evh.sch;
    /* The code below is just a demonstration. */
    /* Increase time and check if there a DP event to execute */
    pthread_mutex_lock( &mutex1 );
    while(!sch->mode){
        pthread_cond_wait( &condition_var, &mutex1 );
    }
    pthread_mutex_unlock( &mutex1 );
    sch->clock += 10; /* Adds one milisecond */
    // printf("Clock %"PRIu64"\n", sch->clock);
    // printf("CONT cur_ev:%p empty? %d Size %ld\n", cur_ev, 
    //        scheduler_is_empty(sch), sch->ev_queue->size);

    /* Get the event but delete it only if executed! */
    if (cur_ev == NULL && !scheduler_is_empty(sch)){
        cur_ev = scheduler_retrieve(sch);
        // printf("Event Type %d\n", cur_ev->type);
        if (cur_ev->type == EVENT_FLOW && cur_ev->time <= sch->clock){
            /* Execute */
            scheduler_delete(sch);
            // printf("CONT executing %d %ld %ld\n", cur_ev->type, cur_ev->time, sch->clock);
            handle_event(&s->evh, cur_ev);
            sim_event_free(cur_ev);
        }
        else if (cur_ev->type == EVENT_OF_MSG_OUT || 
                 cur_ev->type == EVENT_OF_MSG_IN){
            last_ctrl = cur_ev->time;
            // printf("MSG_OF CONT %s %ld\n", cur_ev->type == 4? "OUT":"IN", 
            //        cur_ev->time);
            scheduler_delete(sch);
            handle_event(&s->evh, cur_ev);
            sim_event_free(cur_ev);
        }
        else if (cur_ev->type == EVENT_END && cur_ev->time <= sch->clock) {
            printf("It is over CONT\n");
            pthread_mutex_lock( &mutex1 );
            sch->mode = DES;
            /* Wake up timer */
            pthread_cond_signal( &condition_var );
            pthread_mutex_unlock( &mutex1 );
        }
        cur_ev = NULL;
    }
    /* Check if controller is idle for some time */
    if (sch->clock - last_ctrl > mode_interval){
        printf("Switching to DES %ld, %ld, %ld\n", sch->clock, last_ctrl, 
               sch->clock - last_ctrl);
        pthread_mutex_lock( &mutex1 );
        sch->mode = DES;
        /* Wake up timer */
        pthread_cond_signal( &condition_var );
        pthread_mutex_unlock( &mutex1 );
    }
}

void 
start(struct topology *topo) 
{
    struct sim s;
    memset(&s, 0x0, sizeof(struct sim));
    sim_init(&s, topo, EMU_CTRL);
    // add_flows(topo, SINGLE);
    sim_close(&s);    
}

