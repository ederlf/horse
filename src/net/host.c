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

/* Returns the eth_type of the flow header. 0 in case of none */
static struct netflow* l2_recv_netflow(struct host *h, struct netflow *flow);
static int l3_recv_netflow(struct host *h, struct netflow *flow);

static struct netflow* find_forwarding_ports(struct host *h, struct netflow *flow);
static struct netflow* resolve_mac(struct host *h, struct netflow *flow, uint32_t ip);
static uint32_t  ip_lookup(struct host *h, struct netflow *flow);


struct host {
    struct legacy_node ep; /* Node is an endpoint */
    /* Hash Map of applications */
    struct app *apps;
    /* List of apps to be executed */
    struct exec *execs;
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
    struct port *p = host_port(h, port_id);
    port_add_v4addr(p, addr, netmask);
    /* Add entry to route table */
    struct route_entry_v4 *e = malloc(sizeof(struct route_entry_v4));
    memset(e, 0x0, sizeof(struct route_entry_v4));
    e->ip = addr & netmask;
    e->netmask = netmask;
    e->iface = port_id;
    add_ipv4_entry(&h->ep.rt, e);
}

// void host_handle_netflow(struct node *n, struct netflow *flow)
// {

//     int cont = 0;
//     struct host *h = (struct host*) n;
//     if (!flow->out_ports){
//         cont = host_recv_netflow(h, flow);
//     }
//     if (cont){
//         host_send_netflow(h, flow);
//     }
// }

struct netflow*
host_recv_netflow(struct node *n, struct netflow *flow)
{
    struct host *h = (struct host*) n;
    // log_debug("Received HOST %ld %x\n", h->ep.base.uuid, flow->match.eth_type);
    /* Check MAC and IP addresses. Only returns true if
       both have the node destination MAC and IP       */
    uint16_t eth_type = flow->match.eth_type;
    struct netflow* nf = l2_recv_netflow(h, flow); 
    
    if (eth_type == ETH_TYPE_IP || eth_type == ETH_TYPE_IPV6) {
        if (l3_recv_netflow(h, nf)){
            struct app *app;
            uint16_t ip_proto = flow->match.ip_proto;
            HASH_FIND(hh, h->apps, &ip_proto, sizeof(uint16_t), app);
            /* If gets here send to application */
            log_debug("APP %d %p", ip_proto, app);
            if (app){

                if (app->handle_netflow(nf)){
                    return find_forwarding_ports(h, nf);
                }
            } 
        }
    }
    else if (eth_type == ETH_TYPE_ARP){
        /* End here if there is not further processing */
        return nf;
    }
    // log_debug("No need to continue\n");
    return NULL;
}

void 
host_send_netflow(struct node *n, struct netflow *flow, uint32_t out_port)
{
    
    struct port *p = node_port(n, out_port);
    if (p){
        p->stats.tx_packets += flow->pkt_cnt;
        p->stats.tx_bytes += flow->byte_cnt;
    }
}

static struct netflow* 
l2_recv_netflow(struct host *h, struct netflow *flow){

    struct port *p = host_port(h, flow->match.in_port);
    if (!p) {
        log_debug("No port %d\n", flow->match.in_port);
        return 0;
    }
    uint8_t *eth_dst = flow->match.eth_dst;
    uint8_t bcast_eth_addr[ETH_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    /* Return 0 if destination is other */
    if (memcmp(p->eth_address, eth_dst, ETH_LEN) && 
        memcmp(bcast_eth_addr, eth_dst, ETH_LEN)){
        // log_debug("Leaving " ETH_ADDR_FMT   "\n", ETH_ADDR_ARGS(eth_dst));
        return 0;
    }
    /* Handle ARP */
    if (flow->match.eth_type == ETH_TYPE_ARP){
        /* ARP request */
        if (flow->match.arp_op == ARP_REQUEST){

            /* Returns if port is not the target */
            if (p->ipv4_addr != NULL && 
                flow->match.arp_tpa != p->ipv4_addr->addr){
                return 0;
            } 
            log_debug("Received ARP Request from %x in %x %ld\n", flow->match.arp_spa, flow->match.arp_tpa, flow->start_time);
            struct arp_table_entry *ae = arp_table_lookup(&h->ep.at, flow->match.arp_spa);
            /* Learns MAC of the requester */
            if (!ae){
                // log_debug("Learning %p\n", ae);
                struct arp_table_entry *e = arp_table_entry_new(flow->match.arp_spa, 
                                            flow->match.eth_src, flow->match.in_port);
                arp_table_add_entry(&h->ep.at, e);
            }
            /* Craft reply */
            uint32_t ip_dst = flow->match.arp_spa;
            memcpy(flow->match.arp_sha, p->eth_address, ETH_LEN);
            memcpy(flow->match.arp_tha, flow->match.eth_src, ETH_LEN);
            memcpy(flow->match.eth_dst, flow->match.eth_src, ETH_LEN);
            memcpy(flow->match.eth_src, p->eth_address, ETH_LEN);
            flow->match.arp_spa = flow->match.arp_tpa;
            flow->match.arp_tpa = ip_dst;
            flow->match.arp_op = ARP_REPLY;
            /* Add outport */
            netflow_add_out_port(flow, flow->match.in_port);
            // log_debug("destination eth address " ETH_ADDR_FMT "\n", ETH_ADDR_ARGS(flow->match.eth_src));
        }
        /* ARP Reply */
        else if (flow->match.arp_op == ARP_REPLY) {
            /* Add ARP table */
            uint64_t start_time = flow->start_time;
            log_debug("Received ARP reply %ld\n", flow->start_time);
            struct arp_table_entry *e = arp_table_entry_new(flow->match.arp_spa, 
                                flow->match.arp_sha, flow->match.in_port);
            arp_table_add_entry(&h->ep.at, e);
            /* Cleans ARP flow */
            netflow_destroy(flow);
            flow = NULL;
            /* Check the stack for a flow waiting for the ARP reply */
            if (!node_is_buffer_empty((struct node*) h)){
                flow = node_flow_pop((struct node*) h);
                /* Update the start and end time with the previous ones */ 
                flow->start_time = start_time;
                /* Fill l2 information */
                struct out_port *op;
                LL_FOREACH(flow->out_ports, op) {
                    struct port *p = host_port(h, op->port);
                    memcpy(flow->match.eth_src, p->eth_address, ETH_LEN);
                }
                memcpy(flow->match.eth_dst, e->eth_addr, ETH_LEN);
                /* Start and end time should be equal to the ARP */
                // netflow_update_send_time(*flow, p->curr_speed);
                // log_debug("%p\n", *flow);
                log_debug("To send TYPE %x %ld %p\n", flow->match.eth_type, flow->start_time, flow );
            }
            else {
                /* There is nothing else to send */
                return NULL;
            }
        }
    }
    return flow;
}

static int 
l3_recv_netflow(struct host *h, struct netflow *flow){
    // log_debug("%p\n", flow);
    struct port *p = host_port(h, flow->match.in_port);
    /* IPv4 Assigned and flow is IPv4 */
    if (p->ipv4_addr && flow->match.ipv4_dst){

        return p->ipv4_addr->addr == flow->match.ipv4_dst;
    }
    else if (p->ipv6_addr){
        return !(memcmp(p->ipv6_addr, flow->match.ipv6_dst, IPV6_LEN));
    }
    return 0;
}

static struct netflow* 
find_forwarding_ports(struct host *h, struct netflow *flow)
{
    uint32_t ip = ip_lookup(h, flow);
    /* Only forward if the next hop ip is found */
    if (ip) {
        return resolve_mac(h, flow, ip);    
    }
    return NULL;
}

/* Returns the gateway of destination IP */
static uint32_t 
ip_lookup(struct host *h, struct netflow *flow){
    struct route_entry_v4 *re = ipv4_lookup(&h->ep.rt, flow->match.ipv4_dst);
    if (!re){
        /* Default gateway */
        re = ipv4_lookup(&h->ep.rt, 0);
    }
    if (re){
        netflow_add_out_port(flow, re->iface);
        /* IP of the next hop to search in the ARP table */
        return re->gateway == 0? flow->match.ipv4_dst: re->gateway; 
    } 
    /* Could not find a route */
    return 0;   
}

static struct netflow*
resolve_mac(struct host *h, struct netflow *flow, uint32_t ip){
    struct arp_table_entry *ae = arp_table_lookup(&h->ep.at, ip);
    // log_debug("Looking up MAC for ip %x %p\n", ip, ae);
    if (ae){
        /* Fill Missing information */
        struct out_port *op;
        LL_FOREACH(flow->out_ports, op) {
            struct port *p = host_port(h, op->port);
            flow->match.ipv4_src = p->ipv4_addr->addr;
            memcpy(flow->match.eth_src, p->eth_address, ETH_LEN);
            // netflow_update_send_time(*flow, p->curr_speed);
        }
        memcpy(flow->match.eth_dst, ae->eth_addr, ETH_LEN);
    }
    else {
        /*  ARP request before the flow*/
        uint8_t bcast_eth_addr[ETH_LEN] = {0xff, 0xff, 0xff, 
                                            0xff, 0xff, 0xff};
        /* Craft request */
        struct netflow *arp_req = netflow_new();
        arp_req->match.eth_type = ETH_TYPE_ARP;
        struct out_port *op;
        LL_FOREACH(flow->out_ports, op) {
            struct port *p = host_port(h, op->port);
            /* Need to set missing ip address info */
            flow->match.ipv4_src = p->ipv4_addr->addr;
            memcpy(arp_req->match.eth_src, p->eth_address, ETH_LEN);
            memcpy(arp_req->match.arp_sha, p->eth_address, ETH_LEN);
            arp_req->match.arp_spa = p->ipv4_addr->addr;
            netflow_add_out_port(arp_req, op->port);
        }
        /* Set Destination address */
        memcpy(arp_req->match.eth_dst, bcast_eth_addr, ETH_LEN); 
        // log_debug("IP requested %x\n", ip);
        arp_req->match.arp_tpa = ip;
        arp_req->match.arp_op = ARP_REQUEST;
        arp_req->start_time = flow->start_time;
        arp_req->pkt_cnt = 1;
        arp_req->byte_cnt = 56;
        /* Put the current packet in the queue and flow becomes ARP */ 
        node_flow_push((struct node*) h, flow);
        flow = arp_req;
        // log_debug("%p\n", flow);
    }
    return flow;
}

void 
host_add_app(struct host *h, uint16_t type)
{
    struct app *a =  app_creator(type);
    HASH_ADD(hh, h->apps, type, sizeof(uint16_t), a);
}

void 
host_add_app_exec(struct host *h, uint64_t id, uint32_t type, uint32_t execs_num,
                  uint64_t start_time, void *args, size_t arg_size)
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
        flow = app->start(exec->start_time, exec->args);
        flow->exec_id = exec->id;
        log_debug("Flow Start time APP %ld\n", flow-> start_time);
        exec->exec_cnt -= 1;
        return find_forwarding_ports(h, flow);
        
        // log_debug("%x %p\n", flow->match.eth_type, flow);
        // host_send_netflow((struct node*) h, flow);
        // log_debug("Flow Start time %ld\n", flow.start_time);
        // log_debug("%x\n", flow.match.eth_type);
    }
    return NULL;
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
