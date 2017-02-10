#ifndef NET_FLOW_H
#define NET_FLOW_H 1

#include "flow.h"

enum tcp_flags {
    SYN = 1 << 0,
    ACK = 1 << 1,
    FIN = 1 << 2,
    RST = 1 << 3,
};

struct mpls_stack {
    uint32_t label[MAX_STACK_SIZE];
    int top;
};

struct vlan_stack {
    uint16_t tag[MAX_STACK_SIZE];
    int top;
};

/* Description of a network flow */
struct net_flow {
    uint64_t pkt_cnt;           /* Number of packets in the flow.       */
    uint64_t byte_cnt;          /* Total number of packets in the flow. */
    uint64_t start_time;
    uint64_t end_time;
    struct mpls_stack mpls_stk;
    struct vlan_stack vlan_stk; 
    struct flow_key match;      /* The fields belonging to a flow.      */
    enum tcp_flags tcp_flags;   /* Bitmap of TCP flags present in the flow */
};

#endif