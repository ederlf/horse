/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

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

#include "instruction.h"
#include "instruction_set.h"
#include "packets.h"
#include "ofl.h"
#include <inttypes.h>
#include <stdbool.h>
#include <uthash/uthash.h>

#define MAX_STACK_SIZE 8

/* Description of a switch flow */
struct flow {
    struct ofl_flow_key key;
    struct ofl_flow_key mask;
    uint8_t table_id;
    uint16_t flags;
    uint16_t priority;
    uint64_t cookie;       /* Flow identification. */
    uint64_t pkt_cnt;
    uint64_t byte_cnt;
    uint64_t created; 
    uint64_t hard_timeout;
    uint64_t idle_timeout;
    uint64_t remove_at; /* instruction time + hard timeout */ 
    uint64_t last_used; /* last match, remove if time > last used + idle_timeout */
    struct instruction_set insts;
    UT_hash_handle hh;
};

struct flow *flow_new(void);
void flow_destroy(struct flow *f);
bool flow_key_cmp(struct ofl_flow_key *a, struct ofl_flow_key *b);
void flow_add_instructions(struct flow *f, struct instruction_set is);
void flow_printer(struct flow *f);

void flow_push_vlan(struct flow* f);
void flow_pop_vlan(struct flow* f);
void flow_push_mpls(struct flow* f);
void flow_pop_mpls(struct flow* f);

/* Set field functions */
void set_in_port(struct flow *f, uint32_t in_port);
void set_metadata(struct flow *f, uint64_t metadata);
void set_tunnel_id(struct flow *f, uint64_t tunnel_id);
void set_eth_type(struct flow *f, uint16_t eth_type);
void set_eth_dst(struct flow *f, uint8_t eth_dst[6]);
void set_eth_src(struct flow *f, uint8_t eth_src[6]);
void set_vlan_id(struct flow *f, uint16_t vlan_id);
void set_vlan_pcp(struct flow *f, uint8_t vlan_pcp);
void set_mpls_label(struct flow *f, uint32_t mpls_label);
void set_mpls_tc(struct flow *f, uint8_t mpls_tc);
void set_mpls_bos(struct flow *f, uint8_t mpls_bos);
void set_ip_dscp(struct flow *f, uint8_t ip_dscp);
void set_ip_ecn(struct flow *f, uint8_t ip_ecn);
void set_ip_proto(struct flow *f, uint8_t ip_proto);
void set_ipv4_dst(struct flow *f, uint32_t ipv4_dst);
void set_ipv4_src(struct flow *f, uint32_t ipv4_src);
void set_tcp_dst(struct flow *f, uint16_t tcp_dst);
void set_tcp_src(struct flow *f, uint16_t tcp_src);
void set_sctp_dst(struct flow *f, uint16_t sctp_dst);
void set_sctp_src(struct flow *f, uint16_t sctp_src);
void set_udp_dst(struct flow *f, uint16_t udp_dst);
void set_udp_src(struct flow *f, uint16_t udp_src);
void set_arp_op(struct flow *f, uint16_t arp_op);
void set_icmpv4_type(struct flow *f, uint8_t icmpv4_type);
void set_icmpv4_code(struct flow *f, uint8_t icmpv4_code);
void set_arp_spa(struct flow *f, uint32_t arp_spa);
void set_arp_tpa(struct flow *f, uint32_t arp_tpa);
void set_arp_sha(struct flow *f, uint8_t arp_sha[6]);
void set_arp_tha(struct flow *f, uint8_t arp_tha[6]);
void set_ipv6_dst(struct flow *f, uint8_t ipv6_dst[16]);
void set_ipv6_src(struct flow *f, uint8_t ipv6_src[16]);
void set_ipv6_flabel(struct flow *f, uint32_t ipv6_flabel);
void set_icmpv6_type(struct flow *f, uint8_t icmpv6_type);
void set_icmpv6_code(struct flow *f, uint8_t icmpv6_code);
void set_ipv6_nd_target(struct flow *f, uint8_t ipv6_nd_target[16]);
void set_ipv6_nd_sll(struct flow *f, uint8_t ipv6_nd_sll[6]);
void set_ipv6_nd_tll(struct flow *f, uint8_t ipv6_nd_tll[6]);

/* Set masked field functions */
void set_masked_metadata(struct flow *f, uint64_t metadata, uint64_t mask);
void set_masked_tunnel_id(struct flow *f, uint64_t tunnel_id, uint64_t mask);
void set_masked_eth_dst(struct flow *f, uint8_t eth_dst[6], uint8_t mask[6]);
void set_masked_eth_src(struct flow *f, uint8_t eth_src[6], uint8_t mask[6]);
void set_masked_vlan_id(struct flow *f, uint16_t vlan_id, uint16_t mask);
void set_masked_mpls_label(struct flow *f, uint32_t mpls_label, uint32_t mask);
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

void apply_all_mask(struct ofl_flow_key *key, struct ofl_flow_key *mask);
#endif /* FLOW_H */