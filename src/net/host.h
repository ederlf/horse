#ifndef HOST_H
#define HOST_H 1

#include <inttypes.h>
#include "node.h"
#include "app/app.h"
#include "lib/packets.h"

struct host; 

struct host *host_new(void);

void host_destroy(struct host *h);

void host_add_port(struct host *h, uint32_t port_id, 
                 uint8_t eth_addr[ETH_LEN], uint32_t speed, 
                 uint32_t curr_speed);

void host_set_intf_ipv4(struct host *h, uint32_t port_id, 
                        uint32_t addr, uint32_t netmask);

struct netflow* host_recv_netflow(struct node *n, struct netflow *flow);

void host_send_netflow(struct node *n, struct netflow *flow, 
                       uint32_t out_port);

/* Apps */
void host_add_app(struct host *h, uint16_t type);

void host_add_app_exec(struct host *h, uint64_t id, uint32_t type, uint32_t execs_num, 
                  uint64_t start_time, void *args, size_t arg_size);

struct netflow *host_execute_app(struct host *h, struct exec *exec);

void host_set_default_gw(struct host *h, uint32_t ip, uint32_t port);

void host_set_name(struct host* h, char *name);

/* Access functions*/
char *host_name(struct host *h);
struct port* host_port(const struct host *h, uint32_t port_id);
uint64_t host_uuid(const struct host* h);
struct app *host_apps(const struct host *h);
struct exec *host_execs(const struct host *h);

#endif

