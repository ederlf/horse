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
initial_events(struct sim *s, int test, int cur_time)
{
    struct scheduler *sch = s->evh.sch;
    struct topology *topo = s->evh.topo;
    if (test == LINEAR || test == SINGLE) {
        struct node *cur_node, *tmp;
        HASH_ITER(hh, topology_nodes(topo), cur_node, tmp){
            if (!node_is_buffer_empty(cur_node)){
                struct netflow f = node_flow_pop(cur_node);
                struct sim_event_flow *ev = sim_event_flow_new(f.start_time, cur_node->uuid);
                ev->flow = f;
                 
                scheduler_insert(sch, (struct sim_event*) ev);
            }
        }
        struct sim_event *ev; 
        ev = sim_event_new(UINT64_MAX - 1); 
                scheduler_insert(sch, ev);
        ev->type = EVENT_END;
        scheduler_insert(sch, ev);
    }
    else if (test == CORE_EDGE){
        struct netflow f;
        int i;
        srand(time(NULL)); //108134200
        for (i = cur_time; i < cur_time + EV_NUM; ++i){
            netflow_init(&f);
            f.match.eth_type = 0x800;
            f.match.ipv4_src = random_at_most(254);
            f.match.ipv4_dst = random_at_most(254);
            f.match.in_port = 5;
            f.start_time = f.end_time = i;
            f.pkt_cnt = random_at_most(10);
            f.byte_cnt = f.pkt_cnt * random_at_most(1500);
            struct sim_event_flow *ev = sim_event_flow_new(i, 
                                                   1);
            ev->flow = f;
            scheduler_insert(sch, (struct sim_event*) ev);
        }
        // printf("Will add some events bro\n");
    }
    // struct event *ev; 
    // ev = event_new(UINT64_MAX - 1 , UINT64_MAX - 1); 
    //         scheduler_insert(s->sch, ev);
    // ev->type = EVENT_END;
    // scheduler_insert(s->sch, ev);
}

// static void 
// create_random_events(struct sim *s)
// {
// 	int i;
// 	for (i = 0; i < EV_NUM; ++i){
//         struct event *ev;
//         struct event_flow *flow = event_flow_new(i + 1, 1);
//         if (i == 3){
//             flow->hdr.type = EVENT_CTRL;
//         }
//         flow->flow.start_time = flow->hdr.time;
//         flow->flow.end_time = flow->flow.start_time + 1;
//         memset(&flow->flow.match, 0x0, sizeof(struct flow_key));
//         flow->flow.match.in_port = 1;
//         flow->flow.match.eth_type = 0x800;
//         HASH_ADD(hh, s->events, id, sizeof(uint64_t), 
//                  (struct event_hdr*) flow);
//         ev = event_new(flow->hdr.time , flow->hdr.id); 
//         scheduler_insert(s->sch, ev);
// 	}

// }

static void add_flows(struct topology *topo, int test){
    /* DP 1 */
    if (test == LINEAR) {
        int i;
        for (i = 3; i < 103; ++i) {
            struct datapath *dp = (struct datapath*) topology_node(topo, i);
            struct flow *fl = flow_new();
            struct instruction_set is;
            instruction_set_init(&is);
            // struct write_metadata wm;
            // inst_write_metadata(&wm, 0xbeef);
            // add_write_metadata(&is, wm);
            /* Write actions */
            struct write_actions wa;
            struct action_set as;
            struct action gen_act;
            action_set_init(&as);
            action_output(&gen_act, 2);
            action_set_add(&as, gen_act);
            // action_set_field_u16(&gen_act, SET_IP_PROTO, 6);
            // action_set_add(&as, gen_act);
            inst_write_actions(&wa, as);
            add_write_actions(&is, wa);
            /* Match */
            set_in_port(fl, 1);
            flow_add_instructions(fl, is);
            dp_handle_flow_mod(dp, 0, fl, 0);
        
        /* Flow 2 */
            struct write_actions wa2;
            struct action_set as2;
            struct action gen_act2;
            struct instruction_set is2;
            action_set_init(&as2);
            instruction_set_init(&is2);
            struct flow *f2 = flow_new();
            action_output(&gen_act2, 1);
            action_set_add(&as2, gen_act2);
            inst_write_actions(&wa2, as2);
            add_write_actions(&is2, wa2);
            set_in_port(f2, 2);
            flow_add_instructions(f2, is2);
            dp_handle_flow_mod(dp, 0, f2, 0);    
        }
    }
    else if (test == SINGLE){
            struct datapath *dp = (struct datapath*) topology_node(topo, 1);
            struct flow *fl = flow_new();
            struct instruction_set is;
            instruction_set_init(&is);
            // struct write_metadata wm;
            // inst_write_metadata(&wm, 0xbeef);
            // add_write_metadata(&is, wm);
            /* Write actions */
            struct write_actions wa;
            struct action_set as;
            struct action gen_act;
            action_set_init(&as);
            action_output(&gen_act, CONTROLLER);
            action_set_add(&as, gen_act);
            // action_set_field_u16(&gen_act, SET_IP_PROTO, 6);
            // action_set_add(&as, gen_act);
            inst_write_actions(&wa, as);
            add_write_actions(&is, wa);
            /* Match */
            set_in_port(fl, 1);
            flow_add_instructions(fl, is);
            dp_handle_flow_mod(dp, 0, fl, 0);
        
        /* Flow 2 */
            // struct write_actions wa2;
            // struct action_set as2;
            // struct action gen_act2;
            // struct instruction_set is2;
            // action_set_init(&as2);
            // instruction_set_init(&is2);
            // struct flow *f2 = flow_new();
            // action_output(&gen_act2, 1);
            // action_set_add(&as2, gen_act2);
            // inst_write_actions(&wa2, as2);
            // add_write_actions(&is2, wa2);
            // set_in_port(f2, 2);
            // flow_add_instructions(f2, is2);
            // dp_handle_flow_mod(dp, 0, f2, 0);    
    }
    else if (test == CORE_EDGE) {

    // ip,nw_src=0.0.0.0/0.0.0.64,nw_dst=0.0.0.0/0.0.0.32 actions=output:3
    // ip,nw_src=0.0.0.64/0.0.0.64,nw_dst=0.0.0.32/0.0.0.32 actions=output:4
    // ip,nw_src=0.0.0.0/0.0.0.64,nw_dst=0.0.0.32/0.0.0.32 actions=output:2
    // ip,nw_src=0.0.0.64/0.0.0.64,nw_dst=0.0.0.0/0.0.0.32 actions=output:1
        struct datapath *dp = (struct datapath*) topology_node(topo, 1);
        int i;
        for (i = 0; i < 4; ++i){
            struct flow *fl = flow_new();
            struct instruction_set is;
            instruction_set_init(&is);
            struct write_actions wa;
            struct action_set as;
            struct action gen_act;
            action_set_init(&as);
                      
            /* Match */
            set_eth_type(fl, 0x800);
            if (i == 0){
                action_output(&gen_act, 1);
                set_masked_ipv4_src(fl, 00000040, 00000040);
                set_masked_ipv4_dst(fl, 00000000, 00000020);
            }
            else if (i == 1){
                action_output(&gen_act, 2);
                set_masked_ipv4_src(fl, 00000000, 00000040);
                set_masked_ipv4_dst(fl, 00000020, 00000020);
            }
            else if (i == 2){
                action_output(&gen_act, 3);
                set_masked_ipv4_src(fl, 00000000, 00000040);
                set_masked_ipv4_dst(fl, 00000000, 00000020); 
            }
            else if (i == 3){
                action_output(&gen_act, 4);
                set_masked_ipv4_src(fl, 00000040, 00000040);
                set_masked_ipv4_dst(fl, 00000020, 00000020);
            }
            action_set_add(&as, gen_act);
            inst_write_actions(&wa, as);
            add_write_actions(&is, wa);
            flow_add_instructions(fl, is);
            dp_handle_flow_mod(dp, 0, fl, 0);
        }
        printf("Doing nothing now\n");
    }

    /* DP 2 */
    // dp = (struct datapath*) topology_node(topo, 2);
    // struct flow *fl2 = flow_new();
    // instruction_set_init(&is);
    // struct apply_actions aa;
    // struct action_list al;
    // action_list_init(&al);
    // action_output(&gen_act, 2);
    // action_list_add(&al, gen_act);
    // inst_apply_actions(&aa, al);
    // add_apply_actions(&is, aa);
    // /* Match */
    // set_in_port(fl2, 2);
    // set_eth_type(fl2, 0x800);
    // flow_add_instructions(fl2, is);
    // dp_handle_flow_mod(dp, 0, fl2, 0);
}

static void
sim_init(struct sim *s, struct topology *topo, enum sim_mode mode) 
{
    s->evh.topo = topo;
    s->evh.sch = scheduler_new();
    s->cont.exec = cont_mode;
    init_timer(s->cont, (void*)s);
    set_periodic_timer(/* 1 */1000);
    if (s->mode == EMU_CTRL){
        s->om = of_manager_new();
        of_client_start(s->evh.om->of, false);
    }
    s->mode = mode;
    pthread_create(&s->dataplane, (pthread_attr_t*)0, des_mode, (void*)&s);
}

static void 
sim_close(struct sim *s)
{
    pthread_join(s->dataplane, 0);
    scheduler_destroy(s->evh.sch);
    topology_destroy(s->evh.topo);
}

static pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex1    = PTHREAD_MUTEX_INITIALIZER;
int run = 0;
int last_wrt = 0;

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

        printf("Clock DES %"PRIu64"\n", sch->clock);
        printf("%d\n", ev->type);
        /* Time will be shared now */
        if (sch->clock - last_wrt > 1000000){
            // update_stats(s->topo, s->sch->clock);
            last_wrt = sch->clock;
        }
        // printf("Time %ld\n", s->sch->clock );
        // printf("Run %d\n", run);
        // if (scheduler_is_empty(s->sch)) {
        //     initial_events(s, CORE_EDGE, s->sch->clock);
        //     run += 1;
        //     if (run == 100){
        //         struct event *ev; 
        //         ev = event_new(s->sch->clock  , s->sch->clock ); 
        //         scheduler_insert(s->sch, ev);
        //         ev->type = EVENT_END;
        //         scheduler_insert(s->sch, ev); 
        //         update_stats(s->topo, s->sch->clock);
        //     }
        // }
        if (ev->type == EVENT_FLOW){
            /* Execute */
            handle_event(&s->evh, ev);
            sim_event_free(ev);
        }
        else if( ev->type == EVENT_PACKET_IN ){
            // struct ev* evt = cur_ev;
            // Enqueue(evt);
            /* Gets the next event */
            // printf("DES Pushing to redis %d %ld\n", ev->type, ev->time);
            // redisCommand(c, "RPUSH ctrl_queue %b", ev, (size_t) sizeof(struct event_flow));
            sim_event_free(ev);
            pthread_mutex_lock( &mutex1 );
            sch->mode = CONTINUOUS;
            // redisCommand(c, "HSET dp_signal sig 0");
            /* Wake up timer */
            pthread_cond_signal( &condition_var );
            pthread_mutex_unlock( &mutex1 );
        }
        else if (ev->type == EVENT_END) {
            break;
        }
    }
    /* Awake the timer so it can stop */
    // pthread_mutex_lock( &mutex1 );
    printf("Over timer\n");
    shutdown_timer(s->cont);
    // pthread_cond_signal( &condition_var );
    // pthread_mutex_unlock( &mutex1 );
    return 0;
} 

struct sim_event *cur_ev = NULL;

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
    sch->clock += 1000; /* Adds one milisecond */
    printf("Clock %"PRIu64"\n", sch->clock);
    printf("CONT cur_ev:%p empty? %d Size %ld\n", cur_ev, 
           scheduler_is_empty(sch), sch->ev_queue->size);
    if (cur_ev == NULL && !scheduler_is_empty(sch)){
        cur_ev = scheduler_dispatch(sch);
        
        if (cur_ev->type == EVENT_FLOW && cur_ev->time <= sch->clock){
            /* Execute */
            printf("CONT executing %ld %ld\n", cur_ev->time, sch->clock);
            handle_event(&s->evh, cur_ev);
            sim_event_free(cur_ev);
            cur_ev = NULL;
        }
        else if (cur_ev->type == EVENT_PACKET_IN){
            // printf("CONT Pushing controller queue %d\n", cur_ev->tm);
            // redisCommand(c, "RPUSH ctrl_queue %b", ev, (size_t) sizeof(struct event_flow)); 
            sim_event_free(cur_ev);
            cur_ev = NULL;
        }
        else if (cur_ev->type == EVENT_END && cur_ev->time <= sch->clock) {
            printf("It is over CONT\n");
            pthread_mutex_lock( &mutex1 );
            sch->mode = DES;
            // redisCommand(c, "HSET dp_signal sig 0");
            /* Wake up timer */
            pthread_cond_signal( &condition_var );
            pthread_mutex_unlock( &mutex1 );
        }
    }
    /* TODO: Check if controller is done */
    // printf("Getting signal %d\n", cur_time);
    
}

void 
start(struct topology *topo) 
{
    struct sim s;
    s.mode = EMU_CTRL;
    sim_init(&s, topo, SIM_CTRL);
    add_flows(topo, SINGLE);
    initial_events(&s, SINGLE, 0);    
    sim_close(&s);    
}

