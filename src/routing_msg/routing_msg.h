#ifndef ROUTING_MSG_H
#define ROUTING_MSG_H 1

#include <inttypes.h>

#define HEADER_LEN 8

enum routing_msg_type {
    ROUTER_ACTIVITY = 0,
    ROUTER_FIB = 1,
    BGP_STATE = 2,
    BGP_ANNOUNCE = 3
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

#define BGP_ANNOUNCE_LEN HEADER_LEN + 8
struct bgp_announce {
    struct routing_msg hdr;
    uint32_t peer_rid;
    uint8_t pad[4];  
};

void routing_msg_init(struct routing_msg *msg, uint16_t type,
                      uint16_t size, uint32_t router_id);
struct bgp_state *routing_msg_bgp_state_new(uint32_t router_id,
                                             uint32_t peer_rid, uint8_t state);
struct bgp_announce *routing_msg_bgp_announce_new(uint32_t router_id, uint32_t peer_rid);
uint8_t *routing_msg_pack(struct routing_msg *msg);
void routing_msg_unpack(uint8_t *data, struct routing_msg **msg);

#endif