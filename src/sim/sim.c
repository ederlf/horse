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

#define EV_NUM 10

static void 
create_random_events(struct sim *s)
{
	int i;
	for (i = 0; i < EV_NUM; ++i){
        struct event *ev;
        struct event_flow *flow = event_flow_new(i + 1, 1);
        if (i == 3){
            flow->hdr.type = EVENT_CTRL;
        }
        flow->flow.start_time = flow->hdr.time;
        flow->flow.end_time = flow->flow.start_time + 1;
        memset(&flow->flow.match, 0x0, sizeof(struct flow_key));
        flow->flow.match.in_port = 1;
        flow->flow.match.eth_type = 0x800;
        HASH_ADD(hh, s->events, id, sizeof(uint64_t), 
                 (struct event_hdr*) flow);
        ev = event_new(flow->hdr.time , flow->hdr.id); 
        scheduler_insert(s->sch, ev);
	}

}

static void add_flows(struct topology *topo){
    /* DP 1 */
    struct datapath *dp = (struct datapath*) topology_node(topo, 1);
    struct flow *fl = flow_new();
    struct instruction_set is;
    instruction_set_init(&is);
    struct write_metadata wm;
    inst_write_metadata(&wm, 0xbeef);
    add_write_metadata(&is, wm);
    /* Write actions */
    struct write_actions wa;
    struct action_set as;
    struct action gen_act;
    action_set_init(&as);
    action_output(&gen_act, 2);
    action_set_add(&as, gen_act);
    action_set_field_u16(&gen_act, SET_IP_PROTO, 6);
    action_set_add(&as, gen_act);
    inst_write_actions(&wa, as);
    add_write_actions(&is, wa);
    /* Match */
    set_in_port(fl, 1);
    flow_add_instructions(fl, is);
    dp_handle_flow_mod(dp, 0, fl, 0);
    
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
sim_init(struct sim *s, struct topology *topo) 
{
    s->topo = topo;
    s->events = NULL;
    s->sch = scheduler_new();
}

static void 
sim_clean_ev(struct event_hdr *events)
{
  struct event_hdr *ev, *tmp;
  HASH_ITER(hh, events, ev, tmp) {
      HASH_DEL(events, ev);  
      free(ev);           
  }
}

static void 
sim_close(struct sim *s)
{
    sim_clean_ev(s->events);
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
    struct event_hdr *ev;
    struct sim *s = (struct sim*) args;
    while (!scheduler_is_empty(s->sch)){
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
        struct event *sch_ev = scheduler_dispatch(s->sch);
        s->sch->clock = sch_ev->time;
        HASH_FIND(hh, s->events, &sch_ev->id, sizeof(uint64_t), ev);
        if (ev->type == EVENT_FLOW){
            /* Execute */
            handle_event(s->sch, s->topo, &s->events, ev);
            event_free(sch_ev);
        }
        else if(ev->type == EVENT_CTRL){
            // struct ev* evt = cur_ev;
            // Enqueue(evt);
            /* Gets the next event */
            printf("DES Pushing to redis %ld %ld\n", ev->time, sch_ev->id);
            redisCommand(c, "RPUSH ctrl_queue %b", ev, (size_t) sizeof(*ev));
            event_free(sch_ev);
            pthread_mutex_lock( &mutex1 );
            des = 0;
            redisCommand(c, "HSET dp_signal sig 0");
            /* Wake up timer */
            pthread_cond_signal( &condition_var );
            pthread_mutex_unlock( &mutex1 );
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

    s->sch->clock++;
    redisReply *reply;
    if (cur_ev == NULL && !scheduler_is_empty(s->sch)){
        cur_ev = scheduler_dispatch(s->sch);
    
        HASH_FIND(hh, s->events, &cur_ev->id, sizeof(uint64_t), ev);
        if (ev->type == EVENT_FLOW && cur_ev->time <= s->sch->clock){
            /* Execute */
            printf("CONT executing %ld %ld %ld\n", cur_ev->time, s->sch->clock, cur_ev->id);
            handle_event(s->sch, s->topo, &s->events, ev);
            event_free(cur_ev);
            cur_ev = NULL;
        }
        else if (ev->type == EVENT_CTRL){
            // printf("CONT Pushing controller queue %d\n", cur_ev->tm);
            redisCommand(c, "RPUSH ctrl_queue %b", ev, (size_t) sizeof(*ev)); 
            event_free(cur_ev);
            cur_ev = NULL;
        }
    }
    else {
        redisCommand(c, "HSET dp_signal sig 1");  
    }
    /* Check if controller is done */
    // printf("Getting signal %d\n", cur_time);
    reply = redisCommand(c,"HGET dp_signal sig");
    if (reply){
        printf("REPLY %s\n", reply->str);
        if (strcmp("1", reply->str) == 0){
            // printf("Received signal to awake %s\n", reply->str);
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
    // rt_tessrc/lib/timer.ht();
    sim_init(&s, topo);
    c = redis_connect();
    add_flows(topo);
    create_random_events(&s);
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

