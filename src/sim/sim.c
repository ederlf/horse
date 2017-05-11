/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "net/route_table.h"

#include "sim.h"
#include "event_handler.h"
#include "lib/timer.h"
#include "hiredis/hiredis.h"
#include <unistd.h>
#include <time.h>

#define EV_NUM 10

enum tests {
    SINGLE = 0,
    LINEAR = 1,
    CORE_EDGE = 2,
};

// Assumes 0 <= max <= RAND_MAX
// Returns in the closed interval [0, max]
static uint32_t random_at_most(uint32_t max) {
  uint32_t num_bins = (uint32_t) max + 1;
    uint32_t num_rand = (uint32_t) RAND_MAX + 1;
    uint32_t bin_size = num_rand / num_bins;
    uint32_t defect   = num_rand % num_bins;

  uint32_t x;
  do {
   x = rand();
  }
  // This is carefully written not to overflow
  while (num_rand - defect <= (uint32_t)x);

  // Truncated division is intentional
  return x/bin_size;
}

static void 
initial_events(struct sim *s, int test)
{
    if (test == LINEAR || test == SINGLE) {
        struct node *cur_node, *tmp;
        HASH_ITER(hh, topology_nodes(s->topo), cur_node, tmp){
            if (!node_is_buffer_empty(cur_node)){
                struct netflow f = node_flow_pop(cur_node);
                struct event_flow *ev = event_flow_new(f.start_time, cur_node->uuid);
                ev->flow = f;
                printf("%x\n", ev->flow.match.eth_type); 
                scheduler_insert(s->sch, (struct event*) ev);
            }
        }
    }
    else if (test == CORE_EDGE){
        struct netflow f;
        int i;
        srand(time(NULL)); //108134200
        for (i = 0; i < 10009000; ++i){
            netflow_init(&f);
            f.match.eth_type = 0x800;
            f.match.ipv4_src = random_at_most(254);
            f.match.ipv4_dst = random_at_most(254);
            f.match.in_port = 5;
            f.start_time = f.end_time = i;
            f.pkt_cnt = random_at_most(20);
            f.byte_cnt = f.pkt_cnt * random_at_most(1500);
            struct event_flow *ev = event_flow_new(i, 
                                                   1);
            ev->flow = f;
            scheduler_insert(s->sch, (struct event*) ev);
        }
        printf("Will add some events bro\n");
    }
    struct event *ev; 
    ev = event_new(UINT64_MAX - 1 , UINT64_MAX - 1); 
            scheduler_insert(s->sch, ev);
    ev->type = EVENT_END;
    scheduler_insert(s->sch, ev);
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
            struct datapath *dp = (struct datapath*) topology_node(topo, 0);
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
add_flows_ping(struct topology *topo){
    /* DP 1 */
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
    action_output(&gen_act, 1);
    action_set_add(&as, gen_act);
    // action_set_field_u16(&gen_act, SET_IP_PROTO, 6);
    // action_set_add(&as, gen_act);
    inst_write_actions(&wa, as);
    add_write_actions(&is, wa);
    /* Match */
    // set_in_port(fl, 1);
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

static void
sim_init(struct sim *s, struct topology *topo) 
{
    s->topo = topo;
    s->sch = scheduler_new();
}

static void 
sim_close(struct sim *s)
{
    scheduler_destroy(s->sch);
    topology_destroy(s->topo);
}

// static void
// sim_execute_event(struct sim *s)
// {
// 	struct event_hdr *ev;
// 	struct event *sch_ev = scheduler_dispatch(s->sch);
// 	 Event MUST exist in the hash 
// 	HASH_FIND(hh, s->events, &sch_ev->id, sizeof(uint64_t), ev);
// 	handle_event(s->sch, s->topo, &s->events, ev);
//     event_free(sch_ev);
// }

struct timer t;
redisContext *c;
static pthread_t dataplane;  // thread in which user timer functions 
static pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex1    = PTHREAD_MUTEX_INITIALIZER;
int des = 1;

static void *
dp_thread(void * args){
/*  Executes DES while controller does nothing
    Waits for some event from controller or current time
    is larger than event time */
    struct sim *s = (struct sim*) args;
    while (1){
        // printf("DP %d %d %d\n", cur_time, events[cur_ev].tm, cur_ev);
        /* Only execute in DES mode */
        pthread_mutex_lock( &mutex1 );
        while(!des){
            // printf("Waiting\n");
            pthread_cond_wait( &condition_var, &mutex1 );
        }
        pthread_mutex_unlock( &mutex1 );
        if (scheduler_is_empty(s->sch)){
            continue;
        }
        struct event *ev = scheduler_dispatch(s->sch);
        s->sch->clock = ev->time;
        if (ev->type == EVENT_FLOW){
            /* Execute */
            handle_event(s->sch, s->topo, ev);
            event_free(ev);
        }
        else if( ev->type == EVENT_CTRL ){
            // struct ev* evt = cur_ev;
            // Enqueue(evt);
            /* Gets the next event */
            printf("DES Pushing to redis %d %ld\n", ev->type, ev->time);
            redisCommand(c, "RPUSH ctrl_queue %b", ev, (size_t) sizeof(struct event_flow));
            event_free(ev);
            pthread_mutex_lock( &mutex1 );
            des = 0;
            redisCommand(c, "HSET dp_signal sig 0");
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
    shutdown_timer(t);
    // des = 0;
    // pthread_cond_signal( &condition_var );
    // pthread_mutex_unlock( &mutex1 );
    return 0;
} 

struct event *cur_ev = NULL;

static void 
timer_func(void* args) 
{
    struct event_hdr *ev = NULL;
    
    struct sim *s = (struct sim*) args;
    /* The code below is just a demonstration. */
    /* Increase time and check if there a DP event to execute */
    pthread_mutex_lock( &mutex1 );
    while(des){
        pthread_cond_wait( &condition_var, &mutex1 );
    }
    pthread_mutex_unlock( &mutex1 );

    s->sch->clock += 1000; /* Adds one milisecond */
    redisReply *reply;
    printf("CONT cur_ev:%p empty? %d Size %ld\n", cur_ev, scheduler_is_empty(s->sch), s->sch->ev_queue->size);
    if (cur_ev == NULL && !scheduler_is_empty(s->sch)){
        cur_ev = scheduler_dispatch(s->sch);
        
        if (cur_ev->type == EVENT_FLOW && cur_ev->time <= s->sch->clock){
            /* Execute */
            printf("CONT executing %ld %ld %ld\n", cur_ev->time, s->sch->clock, cur_ev->id);
            handle_event(s->sch, s->topo, cur_ev);
            event_free(cur_ev);
            cur_ev = NULL;
        }
        else if (cur_ev->type == EVENT_CTRL){
            // printf("CONT Pushing controller queue %d\n", cur_ev->tm);
            redisCommand(c, "RPUSH ctrl_queue %b", ev, (size_t) sizeof(struct event_flow)); 
            event_free(cur_ev);
            cur_ev = NULL;
        }
        else if (cur_ev->type == EVENT_END && cur_ev->time <= s->sch->clock) {
            printf("It is over CONT\n");
            redisCommand(c, "HSET dp_signal sig 1");  
        }
    }
    /* Check if controller is done */
    // printf("Getting signal %d\n", cur_time);
    reply = redisCommand(c,"HGET dp_signal sig");
    if (reply){
        printf("REPLY %s\n", reply->str);
        if (strcmp("1", reply->str) == 0){
            // printf("Received signal to awake %s\n", reply->str);
            printf("Adding flows\n");
            add_flows_ping (s->topo);
            pthread_mutex_lock( &mutex1 );
            des = 1;
            pthread_cond_signal( &condition_var );
            pthread_mutex_unlock( &mutex1 );
        }
    }
}

redisContext *redis_connect(void){   
    redisContext *c;
    const char *hostname =  "127.0.0.1";
    int port = 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }
    return c;
}

// typedef void (*app)(void);

// static void func(int x, int y){
//     printf("%d\n", x + y);
// }

// static void rt_test(void){
//     struct route_entry_v4 *e = malloc(sizeof(struct route_entry_v4));
//     struct route_entry_v4 *tmp;
//     struct route_table rt;
//     route_table_init(&rt);
//     e->ip = 0x0a000101;
//     e->netmask = 0xffffff00;
//     e->next_hop = 0x0a000105;
//     add_ipv4_entry(&rt, e);
//     tmp = ipv4_lookup(&rt, 0x0a000202);
//     if (tmp){
//         printf("%x %x\n", tmp->next_hop, tmp->netmask);
//     }
//     else {
//         printf("Not Found\n");
//     }
// }

void 
start(struct topology *topo) 
{
    struct sim s;
    sim_init(&s, topo);
    c = redis_connect();
    add_flows(topo, CORE_EDGE);
    initial_events(&s, CORE_EDGE);
    // create_random_events(&s);
    /* Pass the simulator to the timer */
    t.exec = timer_func;
    init_timer(t, (void*)&s);
    set_periodic_timer(/* 200ms */1000);
    pthread_create(&dataplane, (pthread_attr_t*)0, dp_thread, (void*)&s);
    pthread_join(dataplane, 0);
    // while (!scheduler_is_empty(s.sch)) {
    // 	sim_execute_event(&s);
    // }
    sim_close(&s); 
    redisFree(c);   
}

