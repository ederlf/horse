/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */


#ifndef FLOW_H
#define FLOW_H 1

#include <inttypes.h>
#include <stdbool.h>
#include <uthash/uthash.h>

struct flow_key {
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

struct flow {
    struct flow_key key;
    struct flow_key mask;
    uint32_t priority;
    uint64_t pkt_cnt;
    uint64_t byte_cnt;
    uint64_t created; 
    uint64_t remove_at; 
    uint64_t last_used;
    uint8_t action; /*TODO: it is going to be a list of actions */
    UT_hash_handle hh;
};


struct flow* new_flow(void);
void free_flow(struct flow* f);
bool flow_key_cmp(struct flow_key* a, struct flow_key* b);

/* Set field functions */
void set_in_port(struct flow* f, uint32_t in_port);
void set_metadata(struct flow* f, uint64_t metadata);
void set_tunnel_id(struct flow* f, uint64_t tunnel_id);
void set_eth_type(struct flow* f, uint16_t eth_type);
void set_eth_dst(struct flow* f, uint8_t eth_dst[6]);
void set_eth_src(struct flow* f, uint8_t eth_src[6]);
void set_vlan_id(struct flow* f, uint16_t vlan_id);
void set_vlan_pcp(struct flow* f, uint8_t vlan_pcp);
void set_mpls_label(struct flow* f, uint32_t mpls_label);
void set_mpls_tc(struct flow* f, uint8_t mpls_tc);
void set_mpls_bos(struct flow* f, uint8_t mpls_bos);
void set_ip_dscp(struct flow* f, uint8_t ip_dscp);
void set_ip_ecn(struct flow* f, uint8_t ip_ecn);
void set_ip_proto(struct flow* f, uint8_t ip_proto);
void set_ipv4_dst(struct flow *f, uint32_t ipv4_dst);
void set_ipv4_src(struct flow *f, uint32_t ipv4_src);
void set_tp_dst(struct flow *f, uint16_t tp_dst);
void set_tp_src(struct flow *f, uint16_t tp_src);
void set_arp_op(struct flow *f, uint16_t arp_op);
void set_arp_spa(struct flow *f, uint32_t arp_spa);
void set_arp_tpa(struct flow *f, uint32_t arp_tpa);
void set_arp_sha(struct flow *f, uint8_t arp_sha[6]);
void set_arp_tha(struct flow *f, uint8_t arp_tha[6]);
void set_ipv6_dst(struct flow *f, uint8_t ipv6_dst[16]);
void set_ipv6_src(struct flow *f, uint8_t ipv6_src[16]);
void set_ipv6_nd_target(struct flow *f, uint8_t ipv6_nd_target[16]);
void set_ipv6_nd_sll(struct flow *f, uint8_t ipv6_nd_sll[6]);
void set_ipv6_nd_tll(struct flow *f, uint8_t ipv6_nd_tll[6]);

/* Set masked field functions */
void set_masked_metadata(struct flow* f, uint64_t metadata, uint64_t mask);
void set_masked_tunnel_id(struct flow* f, uint64_t tunnel_id, uint64_t mask);
void set_masked_eth_dst(struct flow* f, uint8_t eth_dst[6], uint8_t mask[6]);
void set_masked_eth_src(struct flow* f, uint8_t eth_src[6], uint8_t mask[6]);
void set_masked_vlan_id(struct flow* f, uint16_t vlan_id, uint16_t mask);
void set_masked_mpls_label(struct flow* f, uint32_t mpls_label, uint32_t mask);
void set_masked_ipv4_dst(struct flow *f, uint32_t ipv4_dst, uint32_t mask);
void set_masked_ipv4_src(struct flow *f, uint32_t ipv4_src, uint32_t mask);
void set_masked_arp_spa(struct flow *f, uint32_t arp_spa, uint32_t mask);
void set_masked_arp_tpa(struct flow *f, uint32_t arp_tpa, uint32_t mask);
void set_masked_arp_sha(struct flow *f, uint8_t arp_sha[6], uint8_t mask[6]);
void set_masked_arp_tha(struct flow *f, uint8_t arp_tha[6], uint8_t mask[6]);
void set_masked_ipv6_dst(struct flow *f, uint8_t ipv6_dst[16], uint8_t mask[16]);
void set_masked_ipv6_src(struct flow *f, uint8_t ipv6_src[16], uint8_t mask[16]);
void set_masked_ipv6_nd_target(struct flow *f, uint8_t ipv6_nd_target[16], uint8_t mask[16]);
void set_masked_ipv6_nd_sll(struct flow *f, uint8_t ipv6_nd_sll[6], uint8_t mask[6]);
void set_masked_ipv6_nd_tll(struct flow *f, uint8_t ipv6_nd_tll[6], uint8_t mask[6]);

void apply_all_mask(struct flow *flow, struct flow_key *mask);
#endif /* FLOW_H */