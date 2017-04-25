#include "ping.h"
#include "lib/packets.h"

void ping_handle_netflow(struct netflow *flow)
{
    uint8_t type = flow->match.icmp_type;
    switch (type){
        /* Received echo request */
        case ECHO: {
            /* Turns netflow into an echo reply */
            uint32_t ip_dst = flow->match.ipv4_src;
            flow->match.icmp_type = ECHO_REPLY;
            flow->match.ipv4_dst = ip_dst;
            break;
        }
        /* Received echo reply */
        case ECHO_REPLY: {
            printf("Received ECHO_REPLY\n");
            break;    
        }
    }
}

static void 
ping_request(struct netflow* flow, uint32_t ip_dst){
    flow->match.ip_proto = IP_PROTO_ICMPV4;
    flow->match.ipv4_dst = ip_dst;
    flow->match.icmp_type = ECHO;
    flow->match.icmp_code = 0;
}

struct netflow ping_start(uint64_t start, void* ip_dst){
    struct netflow n;
    uint32_t *ip = (uint32_t*) ip_dst;
    /* Creates ping request */
    memset(&n, 0x0, sizeof(struct netflow));
    ping_request(&n, *ip);
    n.start_time = start;
    // printf("TIME %ld IP %d %ld\n", start, *ip, sizeof(struct netflow));
    return n;
}



