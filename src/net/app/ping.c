#include "ping.h"
#include "lib/packets.h"

int ping_handle_netflow(struct netflow *flow)
{
    uint8_t type = flow->match.icmpv4_type;
    switch (type){
        /* Received echo request */
        case ECHO: {
            /* Turns netflow into an echo reply */
            printf("Received ECHO REQUEST %x\n", flow->match.ipv4_src);
            uint32_t ip_dst = flow->match.ipv4_src;
            flow->match.icmpv4_type = ECHO_REPLY;
            flow->match.ipv4_dst = ip_dst;
            return 1;
        }
        /* Received echo reply */
        case ECHO_REPLY: {
            printf("Received ECHO_REPLY ms:%lu.%lu\n", flow->end_time / 1000, flow->end_time % 1000);
            return 0;  
        }
    }
    return 0;
}

static void 
ping_request(struct netflow *flow, uint32_t ip_dst){
    netflow_init(flow);
    flow->match.eth_type = ETH_TYPE_IP;
    flow->match.ip_proto = IP_PROTO_ICMPV4;
    flow->match.ipv4_dst = ip_dst;
    flow->match.icmpv4_type = ECHO;
    flow->match.icmpv4_code = 0;
    flow->pkt_cnt = 1;
    flow->byte_cnt = 64;
}

struct netflow ping_start(uint64_t start, void* ip_dst){
    struct netflow n;
    uint32_t *ip = (uint32_t*) ip_dst;
    /* Creates ping request */
    ping_request(&n, *ip);
    n.start_time = start;
    return n;
}



