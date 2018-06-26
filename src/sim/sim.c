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
#include <netemu/netns.h>
#include <uthash/utlist.h>
#include "lib/openflow.h"
#include "lib/signal_handler.h"
#include <log/log.h>
#include "lib/net_utils.h"

#define EV_NUM 1000000


static void *des_mode(void * args);
static void *cont_mode(void* args); 

static pthread_cond_t  mode_cond_var   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx_mode    = PTHREAD_MUTEX_INITIALIZER;
int last_wrt = 0;
uint64_t last_ctrl = 0;
uint64_t last_stats = 0;

static void
create_internal_devices(void)
{
    /* TODO: Move it somewhere else? */
    create_bridge("br0");
    setup_veth(NULL, "conn1", "conn2", "172.20.254.254", "16", "br0");
    /* Add static route */
    netns_run(NULL, "ip route add 172.20.0.0/16 dev conn1");

}

static void 
wait_all_switches_connect(struct topology *topo, struct conn_manager *cm)
{
    uint32_t switches;
    uint32_t time = 0;
    switches = HASH_COUNT(cm->of->active_conns);
    while (switches < topology_dps_num(topo))  {
        sleep(1);
        switches = HASH_COUNT(cm->of->active_conns);
        time++;
        /* Exit if it takes too long to connect */ 
        if (time > 60) {
            fprintf(stderr, "Connection time expired\n");
            exit(1);
        }
    } 
}

// static void 
// wait_all_external_connections(struct topology *topo, struct conn_manager *cm)
// {
//     uint32_t connected;
//     uint32_t time = 0;
//     connected = HASH_COUNT(cm->server->connections);
//     while (connected < topology_routers_num(topo))  {
//         sleep(1);
//         connected = HASH_COUNT(cm->server->connections);
//         time++;
//         /* Exit if it takes too long to connect */ 
//         if (time > 60) {
//             fprintf(stderr, "Connection time expired\n");
//             exit(1);
//         }
//     } 
// }

static void
setup_routers(struct topology *topo)
{
    struct link *l, *ltmp, *added;
    struct router_node *rnode, *rtmp;

    added = NULL;

    HASH_ITER(hh, topology_routers(topo), rnode, rtmp){
        struct router *r = rnode->rt;
        printf("Starting Router %s\n", router_name(r));
        router_start(r);
    }

    HASH_ITER(hh, topology_links(topo), l, ltmp){
        struct node *n1, *n2;
        struct link *done = NULL;
        HASH_FIND(hh, added, &l->node1, sizeof(struct node_port_pair), done);
        if (done){
            continue;
        }
        else {
            n1 = topology_node(topo, l->node1.uuid);
            n2 = topology_node(topo, l->node2.uuid);
            if (n1->type == ROUTER && n2->type == ROUTER){
                char intf1[42], intf2[42];
                struct port *port1, *port2;
                struct router *r1 = (struct router*) n1;
                struct router *r2 = (struct router*) n2;
                uint32_t p1 = l->node1.port;
                uint32_t p2 = l->node2.port;
                sprintf(intf1, "eth%u", p1);
                sprintf(intf2, "eth%u", p2);
                add_veth_pair(intf1, router_name(r1), intf2, router_name(r2));
                port1 = router_port(r1, p1);
                port2 = router_port(r2, p2);
                if (port1){
                    if (port1->ipv4_addr){
                        char ip_str[INET_ADDRSTRLEN], netmask_str[5];
                        struct in_addr ip;
                        int mask;
                        ip.s_addr = htonl(port1->ipv4_addr->addr);
                        mask = count_set_bits(port1->ipv4_addr->netmask);
                        snprintf( netmask_str, (mask / 10) + 2, "%d", mask );
                        get_ip_str(&ip, ip_str, AF_INET);
                        set_intf_ip(router_name(r1), intf1, ip_str,
                                    netmask_str);
                    }
                    
                }
                if (port2){
                    if (port2->ipv4_addr){
                        char ip_str[INET_ADDRSTRLEN], netmask_str[5];
                        struct in_addr ip;
                        int mask;
                        ip.s_addr = htonl(port2->ipv4_addr->addr);
                        mask = count_set_bits(port1->ipv4_addr->netmask);
                        snprintf( netmask_str, (mask / 10) + 2, "%d", mask );
                        get_ip_str(&ip, ip_str, AF_INET);
                        set_intf_ip(router_name(r2), intf2, ip_str,
                                    netmask_str);
                    }
                }
                /* Create a backwards link so it does not repeat*/
                done = (struct link*) xmalloc(sizeof(struct link));
                memset(done, 0x0, sizeof(struct link));
                done->node1.port = l->node2.port;
                done->node1.uuid = l->node2.uuid;
                done->node2.port = l->node1.port;
                done->node2.uuid = l->node1.uuid;
                HASH_ADD(hh, added, node1, sizeof(struct node_port_pair), done);
            }
        }
    }
    // /* Need to think a better way to no go twice over the the routers */
    // HASH_ITER(hh, topology_routers(topo), rnode, rtmp){
    //     struct router *r = rnode->rt;
    //     router_start_protocols(r);
    //     // sleep(1);
    // }

    HASH_ITER(hh, added, l, ltmp){
        HASH_DEL(added, l);
        free(l);
    }
}

static void 
setup(struct sim *s)
{

    struct scheduler *sch = s->evh.sch;
    struct topology *topo = s->evh.topo;
    struct host_node *hnode, *htmp;
    register_handle_sigchild();

    HASH_ITER(hh, topology_hosts(topo), hnode, htmp){
        struct host *h = hnode->h;
        struct exec *exec, *exec_tmp;
        HASH_ITER(hh, host_execs(h), exec, exec_tmp) {
            struct sim_event_app_start *ev = sim_event_app_start_new(
                                            exec->start_time,
                                            host_uuid(h), exec);
            scheduler_insert(sch, (struct sim_event*) ev);
        }
    }
    setup_routers(topo);
    // int i = 0;
    // while (i < 20){
    //     sleep(1);
    //     ++i;
    // }
    /* Event to stop the simulator */
    struct sim_event *end_ev; 
    end_ev = sim_event_new(sim_config_get_end_time(s->config)); 
    end_ev->type = EVENT_END;
    scheduler_insert(sch, end_ev);
}

struct timespec last = {0};
struct timespec now = {0};
FILE *pFile;

static void
sim_init(struct sim *s, struct topology *topo, struct sim_config *config) 
{   

    s->config = config;
    s->evh.topo = topo;
    s->evh.sch = scheduler_new();
    s->evh.live_flows = NULL;
    s->cont.exec = cont_mode; 
    
    if (sim_config_get_mode(s->config) == EMU_CTRL){
        struct dp_node *dpcur, *dptmp;
        s->evh.cm = conn_manager_new(s->evh.sch);
        
        /* Start server if routers included */
        if (topology_routers_num(topo)) {
            struct server *srv = s->evh.cm->srv;
            create_internal_devices();
            server_start(srv);
        }

        /* Add of_settings to client */
        HASH_ITER(hh, topology_datapaths(topo), dpcur, dptmp) {
            struct datapath *dp = dpcur->dp;
            of_client_add_ofsc(s->evh.cm->of, dp_settings(dp));
        }
        of_client_start(s->evh.cm->of, false);
    }

    setup(s);
    wait_all_switches_connect(topo, s->evh.cm);
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
    fclose (pFile);
    scheduler_destroy(s->evh.sch);
    topology_destroy(s->evh.topo);
    conn_manager_destroy(s->evh.cm);
    /* TODO: move somewhere else */
    netns_launch(NULL, "ip link delete conn1");
    netns_launch(NULL, "ip link delete conn2");
    netns_launch(NULL, "ovs-vsctl del-br br0");
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
    /* The code below is just a demonstration. */
    /* Increase time and check if there a DP event to execute */
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

