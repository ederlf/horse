#ifndef RAW_UDP_H
#define RAW_UDP_H 1

#include "lib/netflow.h"

struct raw_udp_args {
    uint64_t duration;  /* How long it lasts? */
    uint64_t rate;      /* How many packets and bytes to be sent */
    uint8_t interval;  /* It basically splits a flow. Value in seconds*/
    uint32_t ip_dst;    /* Destination to send the UDP flows */
    uint16_t dst_port;  /* Destination port */
    uint16_t src_port;   /* Source port */
};

int raw_udp_handle_netflow(struct netflow *flow);
struct netflow raw_udp_start(uint64_t start, void* args);

#endif