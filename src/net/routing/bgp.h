#ifndef BGP_H
#define BGP_H 1

#include "routing.h"
#include "routing_msg.h"

#define MAX_FILE_NAME_SIZE 256

struct neighbor;
struct adv;
struct bgp;

struct bgp* bgp_new(char * config_file);
void bgp_add_neighbor(struct bgp *p, uint32_t neighbor_ip,
                      uint32_t neighbor_as);
void bgp_add_adv_prefix(struct bgp *p, uint32_t prefix, uint8_t cidr);
void bgp_set_router_id(struct bgp* p, uint32_t router_id);
uint32_t bgp_router_id(struct bgp* p);
void bgp_add_local_ip(struct bgp *p, char *ips);

struct routing_msg* bgp_handle_state_msg(struct bgp *p, struct bgp_state *msg);

#endif