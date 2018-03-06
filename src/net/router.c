#include "router.h"
#include "legacy_node.h"

struct router
{
	struct legacy_node rt;
	/* Specific fields may follow */
};

struct router *
router_new(void)
{
    struct router *r = xmalloc(sizeof(struct router));
    legacy_node_init(&r->rt, ROUTER);
    r->rt.base.recv_netflow = router_recv_netflow;
    r->rt.base.send_netflow = router_send_netflow;
    return r;
}

void 
router_destroy(struct router *r)
{
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
router_set_intf_ipv4(struct router *r, uint32_t port_id, 
                   uint32_t addr, uint32_t netmask){
    legacy_node_set_intf_ipv4(&r->rt, port_id, addr, netmask);   
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
