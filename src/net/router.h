#ifndef ROUTER_H
#define ROUTER_H 1

#include "node.h"
#include <uthash/uthash.h>

#define ROUTER_ID_MAX_LEN 61

struct router; 

struct router *router_new(void);
void router_start_protocols(struct router *r);
void router_destroy(struct router *h);
void router_add_port(struct router *h, uint32_t port_id, 
                 uint8_t eth_addr[ETH_LEN], uint32_t speed, 
                 uint32_t curr_speed);
void router_add_protocol(struct router *rt, uint16_t type, char *config_file);
void router_add_port(struct router *r, uint32_t port_id,
                     uint8_t eth_addr[ETH_LEN], uint32_t speed, 
                     uint32_t curr_speed);
void router_set_intf_ipv4(struct router *r, uint32_t port_id,
                          uint32_t addr, uint32_t netmask);
struct netflow* router_recv_netflow(struct node *n, struct netflow *flow);
void router_send_netflow(struct node *n, struct netflow *flow,
                         uint32_t out_port);
void router_handle_control_message(struct router *r, uint8_t *data);
int router_start(struct router *rt);
void router_set_name(struct router* r, char *name);
char *router_name(struct router *r);
struct port* router_port(const struct router *r, uint32_t port_id);
uint64_t router_uuid(const struct router* r);
uint32_t router_id(const struct router *r);
#endif