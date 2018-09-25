#include "setup.h"
#include "server.h"
#include "lib/net_utils.h"
#include "lib/sim_event.h"
#include <netemu/netns.h>
#include <uthash/utlist.h>


static void 
create_internal_devices(void)
{
    create_bridge("br0");
    setup_veth(NULL, "conn1", "conn2", "172.20.254.254", "16", "br0");
    /* Add static route */
    netns_run(NULL, "ip route add 172.20.0.0/16 dev conn1");
}

static void 
setup_app_events(struct topology *topo, struct scheduler *sch)
{
    struct host_node *hnode, *htmp; 
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
   
}

static void 
start_routers(struct topology *topo)
{
    struct router_node *rnode, *rtmp;
    HASH_ITER(hh, topology_routers(topo), rnode, rtmp){
        struct router *r = rnode->rt;
        printf("Starting Router %s\n", router_name(r));
        router_start(r);
    }   
}

static void 
start_router_daemons(struct topology *topo)
{
    struct router_node *rnode, *rtmp;
    HASH_ITER(hh, topology_routers(topo), rnode, rtmp){
        struct router *r = rnode->rt;
        router_start_daemon(r);
    }
}

static void
configure_router_port(struct router *r, struct port *p)
{
    char ip_str[INET_ADDRSTRLEN], netmask_str[5];
    struct in_addr ip;
    int mask;

    ip.s_addr = htonl(p->ipv4_addr->addr);
    mask = count_set_bits(p->ipv4_addr->netmask);
    snprintf( netmask_str, (mask / 10) + 2, "%d", mask );
    get_ip_str(&ip, ip_str, AF_INET);
    set_intf_ip(router_name(r), p->name, ip_str,
                netmask_str);
}

static void
setup_routers(struct topology *topo)
{
    struct link *l, *ltmp, *added;
    
    start_routers(topo);

    added = NULL;

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
                char intf1[MAX_PORT_NAME], intf2[MAX_PORT_NAME];
                struct port *port1, *port2;
                struct router *r1 = (struct router*) n1;
                struct router *r2 = (struct router*) n2;
                uint32_t p1 = l->node1.port;
                uint32_t p2 = l->node2.port;
                sprintf(intf1, "%s-eth%u", router_name(r1), p1);
                sprintf(intf2, "%s-eth%u",router_name(r2), p2);
                add_veth_pair(intf1, router_name(r1), intf2, router_name(r2));
                port1 = router_port(r1, p1);
                port2 = router_port(r2, p2);
                if (port1){
                    port_set_name(port1, intf1);
                    if (port1->ipv4_addr){
                        configure_router_port(r1, port1);
                    }
                    
                }
                if (port2){
                    port_set_name(port2, intf2);
                    if (port2->ipv4_addr){
                        configure_router_port(r2, port2);
                    }
                }
            }
            else {
                /* Because OSPF also needs the host ports, there is need to 
                *  add virtual interfaces to the routers in that case too. 
                */
                struct router *r = NULL;
                struct host *h = NULL;
                uint32_t rp = l->node1.port;
                uint32_t host_port = l->node2.port;
                if (n1->type == ROUTER && n2->type == HOST){
                    r = (struct router*) n1;
                    h = (struct host*) n2;
                    rp = l->node1.port;
                    host_port = l->node2.port;
                }
                else if (n1->type == HOST && n2->type == ROUTER){
                    r = (struct router*) n2;
                    h = (struct host*) n1;
                    rp = l->node2.port;
                    host_port = l->node1.port;
                }
                if (r && h){
                    char intf1[42], intf2[42];
                    struct port *p;
                    sprintf(intf1, "%s-eth%u", router_name(r), rp);
                    sprintf(intf2, "%s-eth%u", host_name(h), host_port);
                    add_veth_pair(intf1, router_name(r), intf2, NULL);
                    set_intf_up(NULL, intf2);
                    p = router_port(r, rp);
                    if (p){
                        char ip_str[INET_ADDRSTRLEN], netmask_str[5];
                        struct in_addr ip;
                        int mask;
                        ip.s_addr = htonl(p->ipv4_addr->addr);
                        mask = count_set_bits(p->ipv4_addr->netmask);
                        snprintf( netmask_str, (mask / 10) + 2, "%d", mask );
                        get_ip_str(&ip, ip_str, AF_INET);
                        set_intf_ip(router_name(r), intf1, ip_str,
                                    netmask_str);
                    }

                }
            }
            /* Create a backwards link so it does not check backwards*/
            done = (struct link*) xmalloc(sizeof(struct link));
            memset(done, 0x0, sizeof(struct link));
            done->node1.port = l->node2.port;
            done->node1.uuid = l->node2.uuid;
            done->node2.port = l->node1.port;
            done->node2.uuid = l->node1.uuid;
            HASH_ADD(hh, added, node1,
                     sizeof(struct node_port_pair), done);
        }
    }

    /* Is there a way to avoid going twice through the routers? */
    start_router_daemons(topo);

    HASH_ITER(hh, added, l, ltmp){
        HASH_DEL(added, l);
        free(l);
    }
}

static void
setup_server(struct conn_manager *cm, struct topology *topo)
{
    struct dp_node *dpcur, *dptmp;
    /* Start server only if routers included */
    if (topology_routers_num(topo)) {
        struct server *srv = cm->srv;
        create_internal_devices();
        server_start(srv);
    }

    /* Add of_settings to client */
    HASH_ITER(hh, topology_datapaths(topo), dpcur, dptmp) {
        struct datapath *dp = dpcur->dp;
        of_client_add_ofsc(cm->of, dp_settings(dp));
    }
    of_client_start(cm->of, false);
}

void 
setup(struct topology *topo, struct scheduler *sch, struct conn_manager *cm)
{
    setup_server(cm, topo);
    setup_app_events(topo, sch);
    setup_routers(topo);
}