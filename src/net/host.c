#include "host.h"
#include "legacy_node.h"
#include "route_table.h"
#include "arp_table.h"
#include "lib/util.h"
#include <log/log.h>
#include <uthash/utlist.h>

#define ETH_ADDR_FMT                                                    \
    "%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8
#define ETH_ADDR_ARGS(ea)                                   \
    (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]

struct host {
    struct legacy_node ep; /* Node is an endpoint */
    struct app *apps;      /* Hash Map of applications */
    struct exec *execs;    /* List of apps to be executed */
};

struct host *
host_new(void)
{
    struct host *h = xmalloc(sizeof(struct host));
    legacy_node_init(&h->ep, HOST);
    h->ep.base.recv_netflow = host_recv_netflow;
    h->ep.base.send_netflow = host_send_netflow;
    h->apps = NULL;
    h->execs = NULL;
    return h;
}

void 
host_destroy(struct host *h)
{
    struct exec *exec, *exec_tmp;
    struct app *app, *app_tmp;
    legacy_node_clean(&h->ep);
    
    HASH_ITER(hh, h->apps, app, app_tmp) {
        HASH_DEL(h->apps, app);
        app_destroy(app);
    }
    HASH_ITER(hh, h->execs, exec, exec_tmp) {
        HASH_DEL(h->execs, exec);
        app_destroy_exec(exec);
    }
    free(h);
}

void 
host_add_port(struct host *h, uint32_t port_id, 
              uint8_t eth_addr[ETH_LEN], uint32_t speed, 
              uint32_t curr_speed)
{   
    node_add_port( (struct node*) h, port_id, eth_addr, speed, curr_speed);
}

void
host_set_intf_ipv4(struct host *h, uint32_t port_id, 
                   uint32_t addr, uint32_t netmask){
    legacy_node_set_intf_ipv4(&h->ep, port_id, addr, netmask);   
}

struct netflow*
host_recv_netflow(struct node *n, struct netflow *flow)
{
    struct host *h = (struct host*) n;
    /* Check MAC and IP addresses. Only returns true if
       both have the node destination MAC and IP       */
    uint16_t eth_type = flow->match.eth_type;
    struct netflow* nf = l2_recv_netflow(&h->ep, flow); 
    if (nf) {
        if (eth_type == ETH_TYPE_IP || eth_type == ETH_TYPE_IPV6) {
            /* A flow arrived here */
            if (is_l3_destination(&h->ep, nf)){
                struct app *app;
                uint16_t ip_proto = flow->match.ip_proto;
                HASH_FIND(hh, h->apps, &ip_proto, sizeof(uint16_t), app);
                /* If gets here send to application */
                log_debug("APP %d %p", ip_proto, app);
                if (app){
                    if (app->handle_netflow(nf)){
                        struct out_port *op;
                        struct netflow *f = find_forwarding_ports(&h->ep, nf,
                                                                  false);
                        if (f != NULL) {
                            LL_FOREACH(flow->out_ports, op) {
                                struct port *p = node_port(&h->ep.base, op->port);
                                f->match.ipv4_src = p->ipv4_addr->addr;
                                memcpy(f->match.eth_src, p->eth_address, ETH_LEN);
                            }
                        }
                        return f;
                    }
                } 
            }
        }
        else if (eth_type == ETH_TYPE_ARP){
            /* There is something after the ARP. Does not seem the ideal flow 
               but needs to be done only in the host. 
            */
            uint16_t new_eth_type = nf->match.eth_type;
            if (nf != NULL && (new_eth_type == ETH_TYPE_IP || 
                new_eth_type == ETH_TYPE_IPV6 )){
                /* Fill l2/l3 information. Shall return a list for multicast?*/
                struct out_port *op;
                LL_FOREACH(nf->out_ports, op) {
                    struct port *p = node_port(&h->ep.base, op->port);
                    nf->match.ipv4_src = p->ipv4_addr->addr;
                    memcpy(nf->match.eth_src, p->eth_address, ETH_LEN);
                }
            }
            return nf;
        }
    }
    return NULL;
}

void 
host_send_netflow(struct node *n, struct netflow *flow, uint32_t out_port)
{
    node_update_port_stats(n, flow, out_port);
}

void 
host_add_app(struct host *h, uint16_t type)
{
    struct app *a =  app_creator(type);
    HASH_ADD(hh, h->apps, type, sizeof(uint16_t), a);
}

void 
host_add_app_exec(struct host *h, uint64_t id, uint32_t type,
                  uint32_t execs_num, uint64_t start_time, void *args, 
                  size_t arg_size)
{
    struct app *app = NULL;
    /* Guarantees the app can be executed */
    HASH_FIND(hh, h->apps, &type, sizeof(uint16_t), app);
    if (app) {
        struct exec *exec = app_new_exec(id, type, execs_num, start_time, 
                                         args, arg_size);
        HASH_ADD(hh, h->execs, id, sizeof(uint64_t), exec);
    }
}

struct netflow* 
host_execute_app(struct host *h, struct exec *exec)
{
    struct app *app;
    HASH_FIND(hh, h->apps, &exec->type, sizeof(uint16_t), app);
    struct netflow *flow = NULL;
    /* If app still has remaining executions */
    if (app) {
        char dst[INET_ADDRSTRLEN];
        struct in_addr ipv4_dst;
        struct out_port *op;
        flow = app->start(exec->start_time, exec->args);
        flow->exec_id = exec->id;
        ipv4_dst.s_addr = ntohl(flow->match.ipv4_dst);
        get_ip_str(ipv4_dst, dst, AF_INET);
        log_info("Flow Start from %s to %s time APP %ld",h->ep.base.name, dst,
                 flow-> start_time);
        exec->exec_cnt -= 1;
        flow = find_forwarding_ports(&h->ep, flow, false);
        if (flow != NULL) {
            LL_FOREACH(flow->out_ports, op) {
                struct port *p = node_port(&h->ep.base, op->port);
                flow->match.ipv4_src = p->ipv4_addr->addr;
                memcpy(flow->match.eth_src, p->eth_address, ETH_LEN);
            }
        }
    }
    return flow;
}

void host_set_default_gw(struct host *h, uint32_t ip, uint32_t port)
{
    struct route_entry_v4 e; 
    memset(&e, 0x0, sizeof(struct route_entry_v4));
    e.ip = 0;
    e.netmask = 0;
    e.gateway = ip;
    e.iface = port;
    add_ipv4_entry(&h->ep.rt, &e);
}

void host_set_name(struct host* h, char *name)
{
    memcpy(h->ep.base.name, name, MAX_NODE_NAME);
}

/* Access functions*/
char *host_name(struct host *h)
{
    return h->ep.base.name;
}

/* Retrieve a datapath port */
struct port* 
host_port(const struct host *h, uint32_t port_id)
{
    struct port *p = node_port( (struct node*) h, port_id);
    return p;
}

uint64_t 
host_uuid(const struct host* h)
{
    return h->ep.base.uuid;
}

struct app *host_apps(const struct host *h)
{
    return h->apps;
}

struct exec *host_execs(const struct host *h)
{
    return h->execs;
}
