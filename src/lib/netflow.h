#ifndef NET_FLOW_H
#define NET_FLOW_H 1

#include "flow.h"
#include "packets.h"

#define STACK_EMPTY -1

enum tcp_flags {
    SYN = 1 << 0,
    ACK = 1 << 1,
    FIN = 1 << 2,
    RST = 1 << 3,
};

struct vlan_stack {
    struct vlan level[MAX_STACK_SIZE];
    int top;
};

struct mpls_stack {
    struct mpls level[MAX_STACK_SIZE];
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

void net_flow_push_vlan(struct net_flow *nf, uint16_t eth_type);
void net_flow_push_mpls(struct net_flow *nf, uint16_t eth_type);
void net_flow_pop_vlan(struct net_flow *nf);
void net_flow_pop_mpls(struct net_flow *nf, uint16_t eth_type);


#endif