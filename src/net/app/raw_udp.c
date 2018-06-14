#include "app.h"
#include "raw_udp.h"
#include "lib/packets.h"
#include <log/log.h>

int raw_udp_handle_netflow(struct netflow *flow)
{
    /* No action is needed when receiving the raw UDP flow */
    char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
    struct in_addr ipv4_src, ipv4_dst;
    ipv4_src.s_addr = ntohl(flow->match.ipv4_src);
    ipv4_dst.s_addr = ntohl(flow->match.ipv4_dst);
    get_ip_str(&ipv4_src, src, AF_INET);
    get_ip_str(&ipv4_dst, dst, AF_INET);
    log_info("Host %s Received UDP flow from %s at %ld\n", dst,
             src, flow->start_time);
    UNUSED(flow);
    return 0;
}

static void 
raw_udp_flow(uint64_t start, struct netflow *flow, 
                  struct raw_udp_args *args){
    
    uint64_t pkt_cnt, byte_cnt;
    uint32_t pkt_size = 1470 + 34; /* Datagram plus headers */
    pkt_cnt = args->rate / (pkt_size * 8);
    byte_cnt =  pkt_cnt * pkt_size;
    flow->match.eth_type = ETH_TYPE_IP;
    flow->match.ip_proto = IP_PROTO_UDP;
    flow->match.ipv4_dst = args->ip_dst;
    flow->match.udp_src = args->src_port;
    flow->match.udp_dst = args->dst_port;
    flow->pkt_cnt = pkt_cnt;
    flow->byte_cnt = byte_cnt;
    flow->start_time = start;
}

struct netflow *raw_udp_start(uint64_t start, void* args){
    struct netflow *n = netflow_new();
    struct raw_udp_args *udp_args = (struct raw_udp_args*) args;
    /* Creates initial udp flow */
    raw_udp_flow(start, n, udp_args);
    return n;
}