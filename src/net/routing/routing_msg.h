#ifndef ROUTING_MSG_H
#define ROUTING_MSG_H 1

#include <inttypes.h>

#define HEADER_LEN 8

enum routing_msg_type {
    BGP_STATE = 0,
    BGP_ANNOUNCE = 1
};

struct routing_msg {
    uint16_t type;
    uint16_t size;
    uint32_t router_id;
};

enum bgp_state_code {
    BGP_STATE_DOWN = 0,
    BGP_STATE_CONNECTED = 1,
    BGP_STATE_UP = 2
};

#define BGP_STATE_LEN HEADER_LEN + 8
struct bgp_state {
    struct routing_msg hdr;
    uint32_t peer_rid;
    uint8_t state;
    uint8_t pad[3];
}; 

struct bgp_community {
    uint32_t as;
    uint32_t local_pref;
};

struct bgp_prefix {
    uint32_t ip;
    uint8_t cidr;
};

#define BGP_ANNOUNCE_LEN HEADER_LEN + 12
struct bgp_announce {
    struct routing_msg hdr;
    uint8_t version;
    uint32_t neighbor; 
    uint8_t origin;
    uint8_t atomic;
    uint32_t med;
/*  Here follows two lists with 
    uint8_t as_path_count;
    uint32_t as[MAX_AS_LEN] 
    uint8_t prefix_count
    struct bgp_prefix prefixes[MAX_PREFIXES]
    Any necessary padding. 
*/

};

void routing_msg_init(struct routing_msg *msg, uint16_t type,
                      uint16_t size, uint32_t router_id);
struct bgp_state * routing_msg_bgp_state_new(uint32_t router_id,
                                             uint32_t peer_rid, uint8_t state);
uint8_t* routing_msg_pack(struct routing_msg *msg);
void routing_msg_unpack(uint8_t *data, struct routing_msg **msg);

#endif