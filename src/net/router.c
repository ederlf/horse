#include "router.h"
#include "legacy_node.h"
#include <netemu/netns.h>
#include <arpa/inet.h>

struct router
{
	struct legacy_node rt;
    struct routing *protocols; /* Hash map of routing protocols */
	/* Specific fields may follow */
};

struct router *
router_new(void)
{
    struct router *r = xmalloc(sizeof(struct router));
    legacy_node_init(&r->rt, ROUTER);
    r->protocols = NULL;
    r->rt.base.recv_netflow = router_recv_netflow;
    r->rt.base.send_netflow = router_send_netflow;
    return r;
}

static void
router_stop(struct router *r){
    char rname[16];
    struct port *p, *tmp, *ports;
    
    memcpy(rname, r->rt.base.name, 16);
    ports = r->rt.base.ports;
    HASH_ITER(hh, ports, p, tmp) {
        netns_run(NULL, "ip link del %s-%s",
                    rname, p->name);
    }
    netns_run(NULL, "pkill exabgp");
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
    char rname[16];
    struct port *p, *tmp, *ports;
    struct routing *rp, *rptmp;
    memcpy(rname, r->rt.base.name, 16);
    if (netns_add(rname)) {
            return -1;
    }
    ports = r->rt.base.ports;
    HASH_ITER(hh, ports, p, tmp) {
        /* Create interfaces and add to namespace*/
        netns_run(NULL, "ip link add %s-%s type veth "
                "peer name node-%s",
                rname, p->name, p->name);
        netns_run(NULL, "ip link set node-%s netns %s",
                p->name, rname);
        netns_run(NULL, "ip link set dev %s-%s up",
                rname, p->name);
        netns_run(rname, "ip link set node-%s name %s",
            p->name, p->name);
        netns_run(rname, "ip link set dev %s up", p->name);
        /* Configure IP */
        if (p->ipv4_addr){
            char addr[INET_ADDRSTRLEN], mask[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(p->ipv4_addr->addr), addr, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(p->ipv4_addr->netmask), mask, INET_ADDRSTRLEN);
            netns_run(rname, "ip addr add %s/%s dev %s", 
                      addr, mask, p->name);
        }
        else if (p->ipv6_addr) {
            char addr[INET6_ADDRSTRLEN], mask[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &(p->ipv6_addr->addr), addr, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &(p->ipv6_addr->netmask), mask,
                      INET6_ADDRSTRLEN);
            netns_run(rname, "ip addr add %s/%s dev %s", 
                      addr, mask, p->name);
        }
    }
    netns_run(rname, "ip link set dev lo up");

    /* Start protocols */
    HASH_ITER(hh, r->protocols, rp, rptmp) {
        rp->start(rp, rname);
    }
    return 0;
}

void 
router_send_netflow(struct node *n, struct netflow *flow,
                         uint32_t out_port)
{
    node_update_port_stats(n, flow, out_port);
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
