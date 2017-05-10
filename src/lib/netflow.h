#ifndef netflow_H
#define netflow_H 1

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

/* List of possible output ports after node processing */
struct out_port {
    uint32_t port;
    struct out_port *next;
};

/* Description of a network flow */
struct netflow {
    uint64_t pkt_cnt;           /* Number of packets in the flow.       */
    uint64_t byte_cnt;          /* Total number of packets in the flow. */
    uint64_t start_time;
    uint64_t end_time;
    struct flow_key match;      /* The fields belonging to a flow.      */
    struct mpls_stack mpls_stk;
    struct vlan_stack vlan_stk; 
    uint8_t tcp_flags;          /* Bitmap of TCP flags present in the flow */
    struct out_port *out_ports; /* List of ports the flow may be sent */
};

void netflow_push_vlan(struct netflow *nf, uint16_t eth_type);
void netflow_push_mpls(struct netflow *nf, uint16_t eth_type);
void netflow_pop_vlan(struct netflow *nf);
void netflow_pop_mpls(struct netflow *nf, uint16_t eth_type);
void netflow_clean_out_ports(struct netflow *flow);
void netflow_update_send_time(struct netflow *flow, uint32_t port_speed);

#endif