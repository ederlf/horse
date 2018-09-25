#include "router.h"
#include "legacy_node.h"
#include "lib/net_utils.h"
#include "routing/routing_daemon.h"
#include "routing_msg/routing_msg.h"
#include <arpa/inet.h>
#include <log/log.h>
#include <netemu/netns.h>

struct router
{
	struct legacy_node ln;
    uint32_t router_id;                /* Highest router_id from protocols */
    bool ecmp;
    struct routing_daemon *daemon;
};

/* The internal network is 172.20.0.0/16, enabling 16384 possible namespaces. 
   The value of the two last bytes increases by one in each namespace.    
*/
struct internal_net_bytes 
{
    uint8_t third;
    uint8_t fourth;
} internal_last_bytes = {0, 0};

static void gen_internal_ip(char *addr);

struct router *
router_new(void)
{
    struct router *r = xmalloc(sizeof(struct router));
    legacy_node_init(&r->ln, ROUTER);
    r->ln.base.recv_netflow = router_recv_netflow;
    r->ln.base.send_netflow = router_send_netflow;
    r->router_id = 0;
    r->ecmp =  false;
    r->daemon = NULL;
    return r;
}

static void
router_stop(struct router *r){
    char rname[MAX_NODE_NAME], internal_intf[MAX_NODE_NAME+10];
    
    memcpy(rname, r->ln.base.name, MAX_NODE_NAME);
    sprintf(internal_intf, "%s-inet-ext", rname);
    delete_intf(internal_intf);
    netns_delete(rname);
    r->daemon->stop(r->daemon);
}

void 
router_destroy(struct router *r)
{
    router_stop(r);
    legacy_node_clean(&r->ln);
    free(r);
}

void 
router_add_port(struct router *r, uint32_t port_id, uint8_t eth_addr[ETH_LEN],
                uint32_t speed, uint32_t curr_speed)
{   
    node_add_port( (struct node*) r, port_id, eth_addr, speed, curr_speed);
}

void 
router_set_quagga_daemon(struct router *r, struct quagga_daemon *d)
{
    r->daemon = (struct routing_daemon*) d;
}

void router_set_exabgp_daemon(struct router *r, struct exabgp_daemon *d)
{
    r->daemon = (struct routing_daemon*) d;
}

void
router_set_intf_ipv4(struct router *r, uint32_t port_id, 
                   uint32_t addr, uint32_t netmask){
    legacy_node_set_intf_ipv4(&r->ln, port_id, addr, netmask);   
}

struct netflow* 
router_recv_netflow(struct node *n, struct netflow *flow)
{
    uint16_t eth_type;
    struct router *r = (struct router*) n;
    struct netflow* nf = NULL;
    uint32_t in_port = flow->match.in_port;
        struct port *p = router_port(r, in_port);
        if (p != NULL) {
            p->stats.rx_bytes += flow->byte_cnt;
            p->stats.rx_packets += flow->pkt_cnt;
    }
    /* L2 handling */
    eth_type = flow->match.eth_type;
    nf = l2_recv_netflow(&r->ln, flow); 
    if (nf) {
        /*L3 handling */
        if (eth_type == ETH_TYPE_IP || eth_type == ETH_TYPE_IPV6) {
            /* Lookup */
            if (is_l3_destination(&r->ln, nf)){
                /* Do we need router apps? */
                return NULL;
            //     struct app *app;
            //     uint16_t ip_proto = flow->match.ip_proto;
            //     HASH_FIND(hh, h->apps, &ip_proto, sizeof(uint16_t), app);
            //     /* If gets here send to application */
            //     log_debug("APP %d %p", ip_proto, app);
            //     if (app){
            //         if (app->handle_netflow(nf)){
            //         }
            //     } 
            }
            else { 
                return find_forwarding_ports(&r->ln, nf, r->ecmp);
            }
        }
        else if (eth_type == ETH_TYPE_ARP) {
            /* End here if there is not further processing */
            return nf;
        }
    }
    return NULL;
}

int 
router_start(struct router *r)
{
    char rname[MAX_NODE_NAME];
    char intf[MAX_NODE_NAME+10], intf2[MAX_NODE_NAME+10];
    char addr[INET_ADDRSTRLEN];
    memcpy(rname, r->ln.base.name, MAX_NODE_NAME);

    if (netns_add(rname)) {
            return -1;
    }
    sprintf(intf2, "%s-inet", rname);
    sprintf(intf,"%s-ext", intf2);
    /* Add internal port */
    gen_internal_ip(addr);
    setup_veth(rname, intf, intf2, addr, "16", "br0");
    netns_run(rname, "ifconfig lo up");
    netns_run(rname, "sysctl -w net.ipv4.ip_forward=1");
    return 0;
}

void 
router_start_daemon(struct router *r)
{
    char router_id[INET_ADDRSTRLEN];
    uint32_t ip = htonl(r->router_id);
    get_ip_str(&ip, router_id, AF_INET);
    r->daemon->start(r->daemon, router_id);
}

void 
router_send_netflow(struct node *n, struct netflow *flow,
                         uint32_t out_port)
{
    node_update_port_stats(n, flow, out_port);
}

static void fib_add(struct router *r, uint8_t *data, size_t len)
{
    uint32_t addr, netmask, next_hop, port_id;
    size_t entries = len / 12; /* Size of each prefix entry */
    uint8_t *pdata = data;
    size_t i;
    for (i = 0; i < entries; ++i) {
        addr = ntohl( *((uint32_t*) pdata) );
        netmask = ntohl( *((uint32_t*) (pdata + 4)) );
        next_hop = ntohl( *((uint32_t*) (pdata + 8)) );
        struct route_entry_v4 *re = ipv4_lookup(&r->ln.rt, next_hop, 0);
        if (re) {
            port_id = re->iface;
        }
        else {
            port_id = 0;
        }
        log_debug("%s Installing route to %x, %x, %x, %x", r->ln.base.name, addr, netmask, next_hop, port_id);
        struct route_entry_v4 *e = malloc(sizeof(struct route_entry_v4));
        memset(e, 0x0, sizeof(struct route_entry_v4));
        e->ip = addr & netmask;
        e->netmask = netmask;
        e->iface = port_id;
        e->gateway = next_hop;
        add_ipv4_entry(&r->ln.rt, e);
        pdata += 12;
    }
}

uint8_t*
router_handle_control_message(struct router *r, uint8_t *data, size_t *ret_len)
{
    struct routing_msg *msg, *msg_ret;
    uint8_t* ret = NULL;
    msg_ret = NULL;
    routing_msg_unpack(data, &msg);
    switch(msg->type) {
        case BGP_STATE:{
            break;
        }
        case BGP_ANNOUNCE:
        case BGP_ACTIVITY:{
            break;
        }
        case BGP_FIB: {
            fib_add(r, data + HEADER_LEN, msg->size -  HEADER_LEN);
            break;
        }
        default:{
            fprintf(stderr, "Cannot process unknown message %d\n", msg->type);  
        }
    }
    if (msg_ret != NULL) {
        *ret_len = msg_ret->size;
        ret = routing_msg_pack(msg_ret);
    }
    free(data);
    free(msg);
    return ret;
}

void 
router_change_daemon_config(struct router *r, char *cmd, uint8_t proto)
{
    r->daemon->change_config(r->daemon, cmd, proto);
}

static void 
gen_internal_ip(char *addr)
{
    /* Increase internal ip */
    if (internal_last_bytes.fourth < 254) {
        internal_last_bytes.fourth++;
    }
    else {
        /* Reached max */
        if(internal_last_bytes.third == 254) {
            return;
        }
        else {
            internal_last_bytes.fourth = 1;
            internal_last_bytes.third++; 
        }
    }
    sprintf(addr, "172.20.%d.%d", internal_last_bytes.third, internal_last_bytes.fourth);
    // set_intf_ip(rname, iface_name, addr, "16");
}

void 
router_set_name(struct router* r, char *name)
{
    memcpy(r->ln.base.name, name, MAX_NODE_NAME);
}

void 
router_set_ecmp(struct router*r, bool enable)
{
    r->ecmp = enable;
}

void 
router_set_id(struct router *r, uint32_t router_id)
{
    r->router_id = router_id;
}

/* Access functions*/
char *
router_name(struct router *r)
{
    return r->ln.base.name;
}

/* Retrieve a datapath port */
struct port* 
router_port(const struct router *r, uint32_t port_id)
{
    struct port *p = node_port( (struct node*) r, port_id);
    return p;
}

uint64_t 
router_uuid(const struct router* r)
{
    return r->ln.base.uuid;
}

uint32_t 
router_id(const struct router *r)
{
    return r->router_id;
}

bool 
router_ecmp(const struct router *r)
{
    return r->ecmp;
}