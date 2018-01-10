#include "app.h"
#include "ping.h"
#include "lib/packets.h"


int ping_handle_netflow(struct netflow *flow)
{
    uint8_t type = flow->match.icmpv4_type;
    switch (type){
        /* Received echo request */
        case ECHO: {
            /* Turns netflow into an echo reply. 
               No need to change the execution id because it uses the same flow.    
            */
            // printf("ECHO REQUEST from:%x to:%x\n", flow->match.ipv4_src, flow->match.ipv4_dst);
            uint32_t ip_dst = flow->match.ipv4_src;
            flow->match.icmpv4_type = ECHO_REPLY;
            flow->match.ipv4_dst = ip_dst;
            netflow_new_flow_id(flow);
             
            return 1;
        }
        /* Received echo reply */
        case ECHO_REPLY: {
            uint64_t start_time;
            memcpy(&start_time, flow->icmp_info.data, sizeof(uint64_t));
            // printf("Start time %ld and End time %ld\n", flow->end_time, start_time );
            uint64_t end_time = flow->start_time - start_time;
            // printf("End time %ld %ld %ld\n", end_time, flow->start_time, start_time  );
            printf("ECHO_REPLY ms:%lu.%lu Src:%x Dst:%x\n", end_time / 1000, end_time % 1000, flow->match.ipv4_dst, flow->match.ipv4_src );
            return 0;  
        }
    }
    return 0;
}

static void 
ping_request(uint64_t start, struct netflow *flow, uint32_t ip_dst){
    flow->match.eth_type = ETH_TYPE_IP;
    flow->match.ip_proto = IP_PROTO_ICMPV4;
    flow->match.ipv4_dst = ip_dst;
    flow->match.icmpv4_type = ECHO;
    flow->match.icmpv4_code = 0;
    flow->pkt_cnt = 1;
    flow->byte_cnt = 64;
    /* For now consider only one ping */
    flow->icmp_info.seq_number = 0;
    flow->icmp_info.identifier = 0x42;
    /* Set the timestamp in the first 8 bytes */
    memcpy(flow->icmp_info.data, &start, sizeof(uint64_t));
    flow->start_time = start;
}

struct netflow *ping_start(uint64_t start, void* ip_dst){
    struct netflow *nf = netflow_new();
    uint32_t *ip = (uint32_t*) ip_dst;
    /* Creates ping request */
    ping_request(start, nf, *ip);
    return nf;
}



