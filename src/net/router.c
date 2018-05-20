#include "router.h"
#include "legacy_node.h"
#include "lib/net_utils.h"
#include "routing/routing.h"
#include "routing/routing_msg.h"
#include <arpa/inet.h>
#include <netemu/netns.h>

struct router
{
	struct legacy_node rt;
    uint32_t router_id;                /* Highest router_id from protocols */
    struct routing *protocols;         /* Hash map of routing protocols */
};

/* The internal network is 172.20.0.0/16, enabling 16384 possible namespaces. 
   The value of the two last bytes increases by one in each namespace.    
*/
struct internal_net_bytes 
{
    uint8_t third;
    uint8_t fourth;
} internal_last_bytes = {0, 0};

static void set_internal_ip(char* rname, char* iface_name);

struct router *
router_new(void)
{
    struct router *r = xmalloc(sizeof(struct router));
    legacy_node_init(&r->rt, ROUTER);
    r->protocols = NULL;
    r->rt.base.recv_netflow = router_recv_netflow;
    r->rt.base.send_netflow = router_send_netflow;
    r->router_id = 0;
    return r;
}

static void
router_stop(struct router *r){
    char rname[MAX_NODE_NAME], internal_intf[MAX_NODE_NAME+10];
    struct routing *rp, *rptmp;
    
    memcpy(rname, r->rt.base.name, MAX_NODE_NAME);
    sprintf(internal_intf, "%s-inet-ext", rname);
    delete_intf(internal_intf);
    HASH_ITER(hh, r->protocols, rp, rptmp) {
        rp->clean(rp, rname);
        HASH_DEL(r->protocols, rp);
        free(rp);
    }

    netns_delete(rname);
}

void 
router_destroy(struct router *r)
{
    router_stop(r);
    legacy_node_clean(&r->rt);
    free(r);
}

void 
router_add_port(struct router *r, uint32_t port_id, uint8_t eth_addr[ETH_LEN],
                uint32_t speed, uint32_t curr_speed)
{   
    node_add_port( (struct node*) r, port_id, eth_addr, speed, curr_speed);
}

void 
router_add_protocol(struct router *rt, uint16_t type, char *config_file)
{
    struct routing *r = routing_factory(type, config_file);
    HASH_ADD(hh, rt->protocols, type, sizeof(uint16_t), r);
    if (r->router_id > rt->router_id){
        rt->router_id = r->router_id;
    }
}

void
router_set_intf_ipv4(struct router *r, uint32_t port_id, 
                   uint32_t addr, uint32_t netmask){
    legacy_node_set_intf_ipv4(&r->rt, port_id, addr, netmask);   
}

struct netflow* 
router_recv_netflow(struct node *n, struct netflow *flow)
{
    struct router *r = (struct router*) n;
    /* L2 handling */
    uint16_t eth_type = flow->match.eth_type;
    struct netflow* nf = l2_recv_netflow(&r->rt, flow); 
    
    if (nf) {
        /*L3 handling */
        if (eth_type == ETH_TYPE_IP || eth_type == ETH_TYPE_IPV6) {
            /* Lookup */
            if (is_l3_destination(&r->rt, nf)){
                /* Do we need router apps? */
            //     struct app *app;
            //     uint16_t ip_proto = flow->match.ip_proto;
            //     HASH_FIND(hh, h->apps, &ip_proto, sizeof(uint16_t), app);
            //     /* If gets here send to application */
            //     log_debug("APP %d %p", ip_proto, app);
            //     if (app){
            //         if (app->handle_netflow(nf)){
            //             return find_forwarding_ports(&h->ep, nf);
            //         }
            //     } 
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
    memcpy(rname, r->rt.base.name, MAX_NODE_NAME);
    if (netns_add(rname)) {
            return -1;
    }
    sprintf(intf2, "%s-inet", rname);
    sprintf(intf,"%s-ext", intf2);
    /* Add internal port */
    setup_veth(rname, intf, intf2, "br0");
    set_internal_ip(rname, intf2);
    set_intf_up(rname, "lo");
    return 0;
}

void 
router_start_protocols(struct router *r)
{
    char rname[MAX_NODE_NAME];
    struct routing *rp, *rptmp;
    memcpy(rname, r->rt.base.name, MAX_NODE_NAME);
    HASH_ITER(hh, r->protocols, rp, rptmp) {
        rp->start(rp, rname);
    }
}

void 
router_send_netflow(struct node *n, struct netflow *flow,
                         uint32_t out_port)
{
    node_update_port_stats(n, flow, out_port);
}

void 
router_handle_control_message(struct router *r, uint8_t *data)
{

    struct routing_msg *msg;
    routing_msg_unpack(data, &msg);
    switch(msg->type) {
        case BGP_STATE:{
            break;
        }
        case BGP_ANNOUNCE:{
            /* Should never happen ?*/
            break;
        }
        default:{
            fprintf(stderr, "Cannot process unknown message %d\n", msg->type);
        }
    }
    UNUSED(r);
    free(data);
    free(msg);
}

static void 
set_internal_ip(char* rname, char* iface_name)
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
    char addr[INET_ADDRSTRLEN];
    sprintf(addr, "172.20.%d.%d", internal_last_bytes.third, internal_last_bytes.fourth);
    set_intf_ip(rname, iface_name, addr, "16");
}

void 
router_set_name(struct router* r, char *name)
{
    memcpy(r->rt.base.name, name, MAX_NODE_NAME);
}

/* Access functions*/
char *
router_name(struct router *r)
{
    return r->rt.base.name;
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
    return r->rt.base.uuid;
}

uint32_t 
router_id(const struct router *r)
{
    return r->router_id;
}