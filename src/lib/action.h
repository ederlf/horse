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
    ACT_COPY_TTL_INWARDS,
    ACT_POP,
    ACT_PUSH_MPLS,
    ACT_PUSH_PBB,
    ACT_PUSH_VLAN,
    ACT_COPY_TTL_OUTWARDS,
    ACT_DECREMENT_TTL,
    ACT_SET_FIELD,
    ACT_QOS,
    ACT_GROUP,
    ACT_OUTPUT
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

struct action_header {
    uint16_t type;
};

/* No limit to size and action types */
struct action_list {
    struct action_header **actions;
    size_t act_num;
};

/* Only one action of each type*/
struct action_set {
    struct action_header *actions[MAX_ACTION_SET];
};

struct output{
    struct action_header header;
    uint32_t port;
    //uint16_t max_len; /* Max size to send to controller.*/                  
};

struct copy_ttl_out{
    struct action_header header;
};

struct copy_ttl_in{
    struct action_header header;
};

struct set_mpls_ttl{
   struct action_header header;
   uint8_t new_ttl;    
};

struct push {
    struct action_header header;
    uint16_t eth_type;
};

struct group {
    struct action_header header;
    uint32_t group_id;
};

struct pop_mpls {
    struct action_header header;
    uint16_t ethertype;
};

struct set_field {
    struct action_header header;
    uint8_t type;
    union {
        uint32_t in_port;
        uint64_t metadata;
        uint64_t tunnel_id;
        uint16_t eth_type;
        uint8_t eth_dst[6]; /* TODO: Use ETH_LEN */
        uint8_t eth_src[6];
        uint16_t vlan_id;
        uint8_t vlan_pcp;
        uint32_t mpls_label;
        uint8_t mpls_tc;
        uint8_t mpls_bos;
        uint8_t ip_dscp;
        uint8_t ip_ecn;
        uint8_t ip_proto;
        uint32_t ipv4_dst;
        uint32_t ipv4_src;
        uint16_t tp_dst;
        uint16_t tp_src;
        uint16_t arp_op;
        uint32_t arp_spa;
        uint32_t arp_tpa;
        uint8_t arp_sha[6];
        uint8_t arp_tha[6];
        uint8_t ipv6_dst[16];
        uint8_t ipv6_src[16];
        uint8_t ipv6_nd_target[16];
        uint8_t ipv6_nd_sll[6];
        uint8_t ipv6_nd_tll[6];
    };
};

struct output* action_new_output(uint32_t port);
void action_list_init(struct action_list *al);
void action_set_init(struct action_set *as);
void action_set_add(struct action_set *as, struct action_header *act, uint8_t type);

#endif