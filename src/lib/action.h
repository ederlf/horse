/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */
#ifndef ACTION_H
#define ACTION_H 1

#include <inttypes.h>
#include "util.h"

#define MAX_ACTION_SET 11

/* TTL fields do not make sense now but
*  are included for possible future usage.
*/
enum action_set_order {
    ACT_METER = 1 << 0,
    ACT_COPY_TTL_INWARDS = 1 << 1,
    ACT_POP_VLAN = 1 << 2,
    ACT_POP_MPLS = 1 << 3,
    ACT_PUSH_MPLS = 1 << 4,
    ACT_PUSH_VLAN = 1 << 5,
    ACT_COPY_TTL_OUTWARDS = 1 << 6,
    ACT_DECREMENT_TTL = 1 << 7,
    ACT_SET_MPLS_TTL = 1 << 8,
    ACT_DECREMENT_MPLS_TTL = 1 << 9,
    ACT_SET_FIELD = 1 << 10,
    ACT_QOS = 1 << 11,
    ACT_GROUP = 1 << 12,
    ACT_OUTPUT = 1 << 13
};

enum set_field_type {
    SET_IN_PORT,
    SET_METADATA,
    SET_TUNNEL_ID,
    SET_ETH_TYPE,
    SET_ETH_DST,
    SET_ETH_SRC,
    SET_VLAN_ID,
    SET_VLAN_PCP,
    SET_MPLS_LABEL,
    SET_MPLS_TC,
    SET_MPLS_BOS,
    SET_IP_DSCP,
    SET_IP_ECN,
    SET_IP_PROTO,
    SET_IPV4_DST,
    SET_IPV4_SRC,
    SET_TCP_DST,
    SET_TCP_SRC,
    SET_UDP_DST,
    SET_UDP_SRC,
    SET_ARP_OP,
    SET_ARP_SPA,
    SET_ARP_TPA,
    SET_ARP_SHA,
    SET_ARP_THA,
    SET_IPV6_DST,
    SET_IPV6_SRC,
    SET_IPV6_ND_TARGET,
    SET_IPV6_ND_SLL,
    SET_IPV6_ND_TLL
};

/* No limit to size and action types */
struct action_list {
    struct action_header **actions;
    size_t act_num;
};

// /* Only one action of each type*/
// struct action_set {
//     struct action_header *actions[MAX_ACTION_SET];
// };

// struct action_header {
//     uint16_t type;
// };

struct meter {
    uint32_t meter_id;
};

struct output{
    uint32_t port;
    //uint16_t max_len; /* Max size to send to controller.*/                  
};

// struct copy_ttl_out{
//     struct action_header header;
// };

// struct copy_ttl_in{
//     struct action_header header;
// };

struct set_mpls_ttl{
   uint8_t new_ttl;    
};

struct push {
    uint16_t eth_type;
};

struct group {
    uint32_t group_id;
};

struct pop_mpls {
    uint16_t eth_type;
};

struct set_field {
    uint8_t field;
    union {
        uint16_t u16_field;
        uint32_t u32_field;
        uint64_t u64_field;
        uint8_t  eth_addr[6];  /* TODO: Use ETH_LEN */
        uint8_t  ipv6_addr[16];
    };
};

struct action {
    uint16_t type;
    union {
        struct meter meter;
        struct output out;
        struct set_mpls_ttl set_mpls_ttl;
        struct group grp;
        struct push psh;
        struct pop_mpls pop_mpls;
        struct set_field set;
    };
};

void action_meter(struct action *meter, uint32_t meter_id);
void action_copy_ttl_in(struct action *copy_ttl_in);
void action_pop_vlan(struct action *pop_vlan);
void action_pop_mpls(struct action *pop, uint16_t eth_type);
void action_push_vlan(struct action *psh, uint16_t eth_type);
void action_push_mpls(struct action *psh, uint16_t eth_type);
void action_copy_ttl_out(struct action *copy_ttl_in);
void action_mpls_ttl(struct action *smttl, uint8_t new_ttl);
void action_dec_ttl(struct action *dec_ttl);
void action_dec_mpls_ttl(struct action *dec_ttl);
void action_set_field_u16(struct action *sf, uint8_t field, uint16_t value);
void action_set_field_u32(struct action *sf, uint8_t field, uint32_t value);
void action_set_field_u64(struct action *sf, uint8_t field, uint64_t value);
void action_set_field_eth_addr(struct action *sf, uint8_t field, uint8_t *eth_addr);
void action_set_field_ipv6_addr(struct action *sf, uint8_t field, uint8_t *ipv6_addr);
void action_group(struct action *grp, uint32_t group_id);
void action_output(struct action *output, uint32_t port);



void action_list_init(struct action_list *al);

#endif