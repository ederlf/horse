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
#include "setup.h"
#include <unistd.h>
#include <time.h>
#include <netemu/netns.h>
#include "lib/openflow.h"
#include "lib/signal_handler.h"
#include <log/log.h>


#define EV_NUM 1000000


static void *des_mode(void * args);
static void *cont_mode(void* args); 

static pthread_cond_t  mode_cond_var   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx_mode    = PTHREAD_MUTEX_INITIALIZER;
int last_wrt = 0;
uint64_t last_ctrl = 0;
uint64_t last_stats = 0;


static void 
wait_all_switches_connect(struct topology *topo, struct conn_manager *cm)
{
    uint32_t switches;
    uint32_t time = 0;
    struct of_conn *ac = cm->of->active_conns;
    switches = HASH_COUNT(ac);
    while (switches < topology_dps_num(topo))  {
        sleep(1);
        ac = cm->of->active_conns;
        switches = HASH_COUNT(ac);
        time++;
        /* Exit if it takes too long to connect */ 
        if (time > 10) {
            fprintf(stderr, "Connection time expired\n");
            exit(1);
        }
    } 
}

struct timespec last = {0};
struct timespec now = {0};
FILE *pFile;

static void
sim_init(struct sim *s, struct topology *topo, struct sim_config *config) 
{   

    s->config = config;
    s->evh.topo = topo;
    s->cont.exec = cont_mode; 
    s->evh.live_flows = NULL;
    s->evh.sch = scheduler_new();

    if (sim_config_get_mode(s->config) == EMU_CTRL){
        s->evh.cm = conn_manager_new(s->evh.sch);
        // s->ebr = emu_bridge_new(s->evh.cm);
        setup(s->evh.topo, s->evh.sch, s->evh.cm, s->ebr);
        if (topology_dps_num(s->evh.topo)){
            wait_all_switches_connect(topo, s->evh.cm);
        }
    }

    struct sim_event *end_ev; 
    end_ev = sim_event_new(sim_config_get_end_time(s->config)); 
    end_ev->type = EVENT_END;
    scheduler_insert(s->evh.sch, end_ev);

    // sleep(5);

    pFile = fopen ("bwm.txt","w");
    init_timer(&s->cont, (void*)s);
    set_periodic_timer(/* 1 */1000);
    clock_gettime(CLOCK_MONOTONIC_RAW, &last);
    pthread_create(&s->dataplane, (pthread_attr_t*)0, des_mode, (void*)s);
}

static void 
sim_close(struct sim *s)
{
    pthread_join(s->dataplane, 0);
    clock_t t; 
    t = clock(); 
    fclose (pFile);
    scheduler_destroy(s->evh.sch);
    conn_manager_destroy(s->evh.cm);
    // emu_bridge_stop(s->ebr);
    
    topology_destroy(s->evh.topo);
    
    /* TODO: move somewhere else */
    netns_launch(NULL, "ip link delete conn1");
    // netns_launch(NULL, "ip link delete conn2");
    netns_launch(NULL, "ifconfig br0 down");
    // netns_launch(NULL, "ovs-vsctl del-br br0");
    netns_launch(NULL, "brctl delbr br0");
    netns_launch(NULL, "ip netns del bridge");
    t = clock() - t; 
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
  
    printf("sim close() took %f seconds to execute \n", time_taken); 
}

static void update_stats(struct topology *topo, uint64_t time){

    struct node *cur_node, *tmp, *nodes;
    nodes = topology_nodes(topo);
    HASH_ITER(hh, nodes, cur_node, tmp) {
        node_write_stats(cur_node, time, pFile);
    }
}

static struct sim_event* 
make_time_msg(uint64_t time){
    uint8_t *buf = xmalloc(sizeof(struct ofp_experimenter_header) + 8);
    struct ofp_experimenter_header *msg = (struct ofp_experimenter_header*) buf;
    msg->header.version = OFP_VERSION; 
    msg->header.type = OFPT_EXPERIMENTER;
    msg->header.length = htons(24);
    msg->header.xid = 0;
    msg->experimenter = htonl(0xF0A1F0A1);
    msg->exp_type = 0;
    uint64_t *t = (uint64_t*) &buf[16];
    *t = hton64(time);   
    /* TODO: Create a dedicated control channel for the simulator messages */
    struct sim_event_fti *ev = sim_event_of_msg_out_new(time,
                                                        0x00000000000000001, 
                                                        buf, 24);
    // scheduler_insert(sch, (struct sim_event*) ev); 
    return (struct sim_event*) ev;
}


static void *
des_mode(void *args){
/*  Executes DES while controller does nothing
    Waits for some event from controller or current time
    is larger than event time */
    struct sim *s = (struct sim*) args;
    struct scheduler *sch = s->evh.sch;   
    while (1){
        /* Only execute in DES mode */
        pthread_mutex_lock( &mtx_mode );
        while(sch->mode){
            pthread_cond_wait( &mode_cond_var, &mtx_mode );
        }
        pthread_mutex_unlock( &mtx_mode );

        struct sim_event *ev = scheduler_dispatch(sch);
        sch->clock = ev->time;
        /* Update status */
        if ((sch->clock - last_stats) / 1000000){
            update_stats(s->evh.topo, sch->clock);
            last_stats = sch->clock;
        }
        /* Need to make it more sane ... */
        if (ev->type != EVENT_END) {
            /* Execute */
            handle_event(&s->evh, ev);
            /* TODO: Very ugly right now*/
            // if ((sch->clock - last_wrt) > 10000000 ){
            //     handle_event(&s->evh, make_time_msg(sch->clock));
            //     /* Wake up timer */
            //     pthread_mutex_lock( &mtx_mode );
            //     last_ctrl = sch->clock;
            //     sch->mode = FTI;
            //     last_wrt = sch->clock;
            //     pthread_cond_signal( &mode_cond_var );
            //     pthread_mutex_unlock( &mtx_mode );
            // }
            if (ev->type == EVENT_FTI) {
                
                /* Wake up timer */
                pthread_mutex_lock( &mtx_mode );
                last_ctrl = sch->clock;
                sch->mode = FTI;
                pthread_cond_signal( &mode_cond_var );
                pthread_mutex_unlock( &mtx_mode );
            
            }
            sim_event_free(ev);
        }
        else {
            log_info("End of the Simulation");
            sim_event_free(ev);
            break;
        }
    }
    shutdown_timer(&s->cont);
    return 0;
} 

struct sim_event *cur_ev = NULL;

static void *
cont_mode(void* args) 
{
    struct sim *s = (struct sim*) args;
    struct scheduler *sch = s->evh.sch;
    /* Increase time and check if there is a DP event to execute */
    pthread_mutex_lock( &mtx_mode );
    while(!sch->mode){
        pthread_cond_wait( &mode_cond_var, &mtx_mode );
    }
    pthread_mutex_unlock( &mtx_mode );
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    uint64_t delta_us = (now.tv_sec - last.tv_sec) * 1000000 + 
                        (now.tv_nsec - last.tv_nsec) / 1000;
    cur_ev = scheduler_retrieve(sch); 
    sch->clock += delta_us;     
    /* Time will be shared now */
    if ((sch->clock - last_wrt) > 10000000){
        handle_event(&s->evh, make_time_msg(sch->clock));
        last_wrt = sch->clock;
    }
    while (cur_ev->time <= sch->clock){
        /* Execute */
        if ((cur_ev->time - last_stats) / 1000000){
            update_stats(s->evh.topo, cur_ev->time);
            last_stats = cur_ev->time ;
        }
        if (cur_ev->type == EVENT_FTI) {
            last_ctrl = cur_ev->time;
        }
        else if(cur_ev->type == EVENT_END){
            goto check_idle;
        }
        handle_event(&s->evh, cur_ev);
        scheduler_delete(sch, cur_ev);
        sim_event_free(cur_ev);
        cur_ev = scheduler_retrieve(sch);
    }

    /* store last time here */
    clock_gettime(CLOCK_MONOTONIC_RAW, &last);
    check_idle:
    /* Check if controller is idle for some time */
    if ( (sch->clock - last_ctrl) > 
        sim_config_get_ctrl_idle_interval(s->config)) {
        pthread_mutex_lock( &mtx_mode );
        sch->mode = DES;
        /* Wake up timer */
        pthread_cond_signal( &mode_cond_var );
        pthread_mutex_unlock( &mtx_mode );
    }
    return 0; 
}

void 
start(struct topology *topo, struct sim_config *config) 
{
    struct sim s; 
    memset(&s, 0x0, sizeof(struct sim));
    sim_init(&s, topo, config);
    sim_close(&s);
}
