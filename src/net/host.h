#ifndef HOST_H
#define HOST_H 1

#include <inttypes.h>
#include "node.hsrc/net/host.c"
#include "lib/packets.h"


enum host_app {
    PING = 0,
};

struct host; 

struct host *host_new(void (*application)(void));
void host_destroy(struct host *h);
void host_add_port(struct host *h, uint32_t port_id, 
                 uint8_t eth_addr[ETH_LEN], uint32_t speed, 
                 uint32_t curr_speed);
struct out_port* host_recv_netflow(struct node *n, struct netflow *flow);
void host_send_netflow(struct node *n, struct netflow *flow, 
                     uint32_t port);
struct port* host_port(const struct host *h, uint32_t port_id);
uint64_t host_uuid(const struct host* h);

#endif

