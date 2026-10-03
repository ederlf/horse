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

struct tag {
    uint16_t type;
    union {
        struct vlan vlan_tag;
        struct mpls mpls_tag;
    };
};

struct tag_stack {
    struct tag level[MAX_STACK_SIZE];
    int top;
};

/* List of possible output ports after node processing */
struct out_port {
    uint32_t port;
    struct out_port *next;
};


/* Metadata of a netflow from the datapath */
struct dp_meta {
    uint32_t buffer_id;     /* ID assigned by datapath. */
    uint8_t reason;         /* Reason packet is being sent (one of OFPR_*) */
    uint8_t table_id;       /* ID of the table that was looked up */
    uint64_t cookie;        /* Cookie of the flow entry that was looked up. */
};

/* Description of a network flow */
struct netflow {
    uint64_t pkt_cnt;           /* Number of packets in the flow.           */
    uint64_t byte_cnt;          /* Total number of packets in the flow.     */
    uint64_t start_time;
    uint64_t exec_id;           /* Tracks the app that triggered it*/
    uint32_t rate;              /* Current rate of the flow                 */
    struct ofl_flow_key match;      /* The fields belonging to a flow.      */
    struct tag_stack tags;
    
    union { 
        uint16_t tcp_flags;          /* Bitmap of TCP flags present 
                                       in the flow  */
        struct icmp_echo_reply icmp_info;
    }; 
    uint8_t *payload;            /* Possible payload */ 
    uint16_t payload_len;        /* Size of the payload */
    struct dp_meta metadata;    /* Any additional information. e.g: pkt out */
    struct out_port *out_ports; /* List of ports the flow may be sent       */
};

struct netflow *netflow_new(void);
struct netflow *netflow_new_from_netflow(struct netflow *to_copy);
void netflow_init(struct netflow *nf);
void netflow_destroy(struct netflow *nf);
void netflow_push_vlan(struct netflow *nf, uint16_t eth_type);
void netflow_push_mpls(struct netflow *nf, uint16_t eth_type);
void netflow_pop_vlan(struct netflow *nf);
bool netflow_is_outer_mpls(struct netflow *nf);
void netflow_pop_mpls(struct netflow *nf, uint16_t eth_type);
bool netflow_is_vlan_tagged(struct netflow *nf);
size_t netflow_to_pkt(struct netflow *nf, uint8_t *buffer);
void pkt_to_netflow(uint8_t *buffer, struct netflow *nf, size_t pkt_len);
void netflow_clean_out_ports(struct netflow *nf);
uint32_t netflow_calculate_hash(struct netflow *nf);
void netflow_add_out_port(struct netflow *nf, uint32_t out_port);
void netflow_update_send_time(struct netflow *nf, uint32_t port_speed);

#endif