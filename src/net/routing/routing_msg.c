#include "routing_msg.h"
#include "lib/util.h"
#include <arpa/inet.h>

void routing_msg_init(struct routing_msg *msg, uint16_t type, uint16_t size,
                      uint32_t router_id)
{
    msg->type = type;
    msg->size = size;
    msg->router_id = router_id;
}

struct bgp_state * 
routing_msg_bgp_state_new(uint32_t router_id, uint32_t peer_rid, uint8_t state)
{
    struct bgp_state *msg = xmalloc(sizeof(struct bgp_state));
    routing_msg_init((struct routing_msg*) msg, BGP_STATE, BGP_STATE_LEN,
                     router_id);
    msg->peer_rid = peer_rid;
    msg->state = state;
    return msg;
}

struct bgp_announce *
routing_msg_bgp_announce_new(uint32_t router_id, uint32_t peer_rid)
{
    struct bgp_announce *msg = xmalloc(sizeof(struct bgp_announce));
    memset(msg, 0x0, sizeof(struct bgp_announce));
    routing_msg_init((struct routing_msg*) msg, BGP_ANNOUNCE, BGP_ANNOUNCE_LEN,
                     router_id);
    msg->peer_rid = peer_rid;
    return msg;
}

static uint8_t*
pack_msg_hdr(struct routing_msg *msg)
{
    uint8_t *data = (uint8_t*) xmalloc(msg->size);
    struct routing_msg *pack = (struct routing_msg*) data;
    pack->type = htons(msg->type);
    pack->size = htons(msg->size);
    pack->router_id = htonl(msg->router_id);
    return data;
}

static void
pack_bgp_state(struct routing_msg *msg, uint8_t* data)
{
    struct bgp_state *s = (struct bgp_state*) msg;
    struct bgp_state *pack = (struct bgp_state*) data;
    pack->peer_rid = htonl(s->peer_rid);
    pack->state = s->state;
    memset(pack->pad, 0x0, 3);
}

static void
pack_bgp_announce(struct routing_msg *msg, uint8_t* data)
{
    struct bgp_announce *s = (struct bgp_announce*) msg;
    struct bgp_announce *pack = (struct bgp_announce*) data;
    pack->peer_rid = htonl(s->peer_rid);
    memset(pack->pad, 0x0, 4);
}

uint8_t* 
routing_msg_pack(struct routing_msg *msg)
{
    uint8_t *data = pack_msg_hdr(msg);
    switch(msg->type) {
        case BGP_STATE:{
            pack_bgp_state(msg, data);
            break;
        } 
        case BGP_ANNOUNCE:{
            pack_bgp_announce(msg, data);
            break;
        }
        default: {
            fprintf(stderr, "Failed to pack. Unknown message %d\n", msg->type);
            return NULL;
        }
    }
    return data;
}

/* Unpacking */

static void
unpack_bgp_state(uint8_t* data, struct routing_msg **msg)
{
    struct bgp_state *s = xmalloc(sizeof(struct bgp_state));
    struct bgp_state *unpack = (struct bgp_state*) data;
    s->peer_rid = ntohl(unpack->peer_rid);
    s->state = unpack->state;
    *msg = (struct routing_msg*) s;
}

void 
routing_msg_unpack(uint8_t *data, struct routing_msg **msg)
{
    struct routing_msg *unpack = (struct routing_msg*) data;
    switch(ntohs(unpack->type)) {
        case BGP_STATE: {
            unpack_bgp_state(data, msg);
            break;
        }
        case  BGP_ANNOUNCE: {
            break;
        }
        default: {
            fprintf(stderr, "Failed to unpack. Unknown message %d\n",
                    ntohs(unpack->type));
        }
    } 
    (*msg)->type = ntohs(unpack->type);
    (*msg)->size = ntohs(unpack->size);
    (*msg)->router_id = ntohl(unpack->router_id);
}



// int main(int argc, char const *argv[])
// {
//     struct bgp_state *msg = routing_msg_bgp_state_new(16777226, 33554442,
//                                                           BGP_STATE_UP); 
//     uint8_t *data =  routing_msg_pack((struct routing_msg* ) msg);
//     int i;
//     for (i = 0; i < 16; ++i){
//         printf("%x", data[i]);
//     }
//     printf("\n");
//     struct bgp_state *msg2;
//     routing_msg_unpack(data, (struct routing_msg**) &msg2);
//     printf("%u, %u, %d\n", msg2->hdr.router_id, msg2->peer_rid, msg2->state);
//     free(msg);
//     free(msg2);
//     free(data);
//     return 0;
// }