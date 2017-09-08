/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */


#include "flow.h"
#include "util.h"
#include <string.h>

#define ALL_UINT8_MASK 0xff
#define ALL_UINT16_MASK 0xffff
#define ALL_UINT32_MASK 0xffffffff
#define ALL_UINT64_MASK 0xffffffffffffffff

#define ETH_ADDR_FMT                                                    \
    "%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8

#define ETH_ADDR_ARGS(ea)                                   \
    (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]

static
void init_instruction_set(struct flow *f) {
    memset(&f->insts, 0x0, sizeof(struct instruction_set));
}

static void
apply_mask_eth_addr(uint8_t addr[6], uint8_t mask[6])
{
    uint32_t *v32, *m32;
    uint16_t *v16, *m16;

    v32 = (uint32_t*)  &addr[0];
    v16 = (uint16_t*)  &addr[4];
    m32 = (uint32_t*)  &mask[0];
    m16 = (uint16_t*)  &mask[4];
    *v32 = *((uint32_t*)&addr[0]) & *m32;
    *v16 = *((uint16_t*)&addr[4]) & *m16;
} 

static void
apply_mask_ipv6(uint8_t addr[16], uint8_t mask[16])
{
    uint64_t *v1_64, *m1_64;
    uint64_t *v2_64, *m2_64;

    v1_64 = (uint64_t*)  &addr[0];
    v2_64 = (uint64_t*)  &addr[8];
    m1_64 = (uint64_t*)  &mask[0];
    m2_64 = (uint64_t*)  &mask[8];
    *v1_64 = *((uint64_t*)&addr[0]) & *m1_64;
    *v2_64 = *((uint64_t*)&addr[8]) & *m2_64;
} 

/* Allocates a new flow and sets all fields to 0 */
struct flow*
flow_new(void)
{
    struct flow *f = xmalloc(sizeof(struct flow));
    f->table_id = 0;
    f->priority = 0;
    f->cookie = 0;
    f->pkt_cnt = 0;
    f->byte_cnt = 0;
    f->hard_timeout = 0;
    f->idle_timeout = 0;
    f->created = 0;
    f->remove_at = 0;
    f->last_used = 0;
    memset(&f->key, 0x0, sizeof(struct ofl_flow_key));
    memset(&f->mask, 0x0, sizeof(struct ofl_flow_key));
    init_instruction_set(f);
    return f;
}

void
flow_destroy(struct flow *f)
{
    instruction_set_clean(&f->insts);
    free(f);
}

bool
flow_key_cmp(struct ofl_flow_key *a, struct ofl_flow_key *b)
{
    return  ( memcmp(a, b, sizeof(struct ofl_flow_key) ) == 0);
}

void
flow_add_instructions(struct flow *f, struct instruction_set insts)
{
    f->insts = insts;
}

void flow_printer(struct flow *f) {
    printf("table_id:%d eth_dst="ETH_ADDR_FMT" in_port=%d, pkt_cnt:%ld, priority:%d \n", f->table_id, ETH_ADDR_ARGS(f->key.eth_dst), f->key.in_port, f->pkt_cnt, f->priority);
}

void
set_in_port(struct flow *f, uint32_t in_port)
{
    f->key.in_port = in_port;
    f->mask.in_port = ALL_UINT32_MASK;
}

void
set_metadata(struct flow *f, uint64_t metadata)
{
    f->key.metadata = metadata;
    f->mask.metadata = ALL_UINT64_MASK;
}

void
set_tunnel_id(struct flow *f, uint64_t tunnel_id)
{
    f->key.tunnel_id = tunnel_id;
    f->mask.tunnel_id = ALL_UINT64_MASK;
}

void
set_eth_type(struct flow *f, uint16_t eth_type)
{
    f->key.eth_type = eth_type;
    f->mask.eth_type = ALL_UINT16_MASK;
}

void
set_eth_dst(struct flow *f, uint8_t eth_dst[6])
{
    memcpy(f->key.eth_dst, eth_dst, 6);
    memset(f->mask.eth_dst, 0xff, 6);
}

void
set_eth_src(struct flow *f, uint8_t eth_src[6])
{
    memcpy(f->key.eth_src, eth_src, 6);
    memset(f->mask.eth_src, 0xff, 6);
}

void
set_vlan_id(struct flow *f, uint16_t vlan_id)
{
    f->key.vlan_id = vlan_id;
    f->mask.vlan_id = ALL_UINT16_MASK;
}

void
set_vlan_pcp(struct flow *f, uint8_t vlan_pcp)
{
    f->key.vlan_pcp = vlan_pcp;
    f->mask.vlan_pcp = ALL_UINT8_MASK;
}

void
set_mpls_label(struct flow *f, uint32_t mpls_label)
{
    f->key.mpls_label = mpls_label;
    f->mask.mpls_label = ALL_UINT32_MASK;
}

void
set_mpls_tc(struct flow *f, uint8_t mpls_tc)
{
    f->key.mpls_tc = mpls_tc;
    f->mask.mpls_tc = ALL_UINT8_MASK;
}

void
set_mpls_bos(struct flow *f, uint8_t mpls_bos)
{
    f->key.mpls_bos = mpls_bos;
    f->mask.mpls_bos = ALL_UINT8_MASK;
}

void
set_ip_dscp(struct flow *f, uint8_t ip_dscp)
{
    f->key.ip_dscp = ip_dscp;
    f->mask.ip_dscp = ALL_UINT8_MASK;
}

void
set_ip_ecn(struct flow *f, uint8_t ip_ecn)
{
    f->key.ip_ecn = ip_ecn;
    f->mask.ip_ecn = ALL_UINT8_MASK;
}

void
set_ip_proto(struct flow *f, uint8_t ip_proto)
{
    f->key.ip_proto = ip_proto;
    f->mask.ip_proto = ALL_UINT8_MASK;
}

void
set_ipv4_dst(struct flow *f, uint32_t ipv4_dst)
{
    f->key.ipv4_dst = ipv4_dst;
    f->mask.ipv4_dst = ALL_UINT32_MASK;
}

void
set_ipv4_src(struct flow *f, uint32_t ipv4_src)
{
    f->key.ipv4_src = ipv4_src;
    f->mask.ipv4_src = ALL_UINT32_MASK;
}

void
set_tcp_dst(struct flow *f, uint16_t tcp_dst)
{
    f->key.tcp_dst = tcp_dst;
    f->mask.tcp_dst = ALL_UINT16_MASK;
}

void
set_tcp_src(struct flow *f, uint16_t tcp_src)
{
    f->key.tcp_src = tcp_src;
    f->mask.tcp_src = ALL_UINT16_MASK;
}

void
set_sctp_dst(struct flow *f, uint16_t sctp_dst)
{
    f->key.sctp_dst = sctp_dst;
    f->mask.sctp_dst = ALL_UINT16_MASK;
}

void
set_sctp_src(struct flow *f, uint16_t sctp_src)
{
    f->key.sctp_src = sctp_src;
    f->mask.sctp_src = ALL_UINT16_MASK;
}

void
set_udp_dst(struct flow *f, uint16_t udp_dst)
{
    f->key.udp_dst = udp_dst;
    f->mask.udp_dst = ALL_UINT16_MASK;
}

void set_udp_src(struct flow *f, uint16_t udp_src)
{
    f->key.udp_src = udp_src;
    f->mask.udp_src = ALL_UINT16_MASK;
}


void
set_icmpv4_type(struct flow *f, uint8_t icmpv4_type)
{
    f->key.icmpv4_type = icmpv4_type;
    f->mask.icmpv4_type = ALL_UINT8_MASK;
}

void
set_icmpv4_code(struct flow *f, uint8_t icmpv4_code)
{
    f->key.icmpv4_code = icmpv4_code;
    f->mask.icmpv4_code = ALL_UINT8_MASK;
}

void set_arp_op(struct flow *f, uint16_t arp_op)
{
    f->key.arp_op = arp_op;
    f->mask.arp_op = ALL_UINT16_MASK;
}

void
set_arp_spa(struct flow *f, uint32_t arp_spa)
{
    f->key.arp_spa = arp_spa;
    f->mask.arp_spa = ALL_UINT32_MASK;
}

void
set_arp_tpa(struct flow *f, uint32_t arp_tpa)
{
    f->key.arp_tpa = arp_tpa;
    f->mask.arp_tpa = ALL_UINT32_MASK;
}

void
set_arp_sha(struct flow *f, uint8_t arp_sha[6])
{
    memcpy(f->key.arp_sha, arp_sha, 6);
    memset(f->mask.arp_sha, 0xff, 6);
}

void
set_arp_tha(struct flow *f, uint8_t arp_tha[6])
{
    memcpy(f->key.arp_tha, arp_tha, 6);
    memset(f->mask.arp_tha, 0xff, 6);
}

void
set_ipv6_dst(struct flow *f, uint8_t ipv6_dst[16])
{
    memcpy(f->key.ipv6_dst, ipv6_dst, 16);
    memset(f->mask.ipv6_dst, 0xff, 16);
}

void
set_ipv6_src(struct flow *f, uint8_t ipv6_src[16])
{
    memcpy(f->key.ipv6_src, ipv6_src, 16);
    memset(f->mask.ipv6_src, 0xff, 16);
}

void
set_ipv6_flabel(struct flow *f, uint32_t ipv6_flabel)
{
    f->key.ipv6_flabel = ipv6_flabel;
    f->mask.ipv6_flabel = ALL_UINT32_MASK;
}

void
set_icmpv6_type(struct flow *f, uint8_t icmpv6_type)
{
    f->key.icmpv6_type = icmpv6_type;
    f->mask.icmpv6_type = ALL_UINT8_MASK;
}

void
set_icmpv6_code(struct flow *f, uint8_t icmpv6_code)
{
    f->key.icmpv6_code = icmpv6_code;
    f->mask.icmpv6_code = ALL_UINT8_MASK;
}

void
set_ipv6_nd_target(struct flow *f, uint8_t ipv6_nd_target[16])
{
    memcpy(f->key.ipv6_nd_target, ipv6_nd_target, 16);
    memset(f->mask.ipv6_nd_target, 0xff, 16);
}

void
set_ipv6_nd_sll(struct flow *f, uint8_t ipv6_nd_sll[6])
{
    memcpy(f->key.ipv6_nd_sll, ipv6_nd_sll, 6);
    memset(f->mask.ipv6_nd_sll, 0xff, 6);
}

void
set_ipv6_nd_tll(struct flow *f, uint8_t ipv6_nd_tll[6])
{
    memcpy(f->key.ipv6_nd_tll, ipv6_nd_tll, 6);
    memset(f->mask.ipv6_nd_tll, 0xff, 6);
}

/* Start of set masked functions */

void flow_set_masked_metadata(struct flow *f, uint64_t metadata,
                              uint64_t mask)
{
    f->key.metadata = metadata & mask;
    f->mask.in_port = mask;
}

void flow_set_masked_tunnel_id(struct flow *f, uint64_t tunnel_id, uint64_t mask)
{
    f->key.tunnel_id = tunnel_id & mask;
    f->mask.tunnel_id = mask;
}

void flow_set_masked_eth_dst(struct flow *f, uint8_t eth_dst[6], uint8_t mask[6])
{
    memcpy(f->mask.eth_dst, mask, 6);
    memcpy(f->key.eth_dst, eth_dst, 6);
    apply_mask_eth_addr(f->key.eth_dst, mask);
}

void flow_set_masked_eth_src(struct flow *f, uint8_t eth_src[6], uint8_t mask[6])
{
    memcpy(f->mask.eth_src, mask, 6);
    memcpy(f->key.eth_src, eth_src, 6);
    apply_mask_eth_addr(f->key.eth_src, mask);
}

void flow_set_masked_vlan_id(struct flow *f, uint16_t vlan_id, uint16_t mask)
{
    f->key.vlan_id = vlan_id & mask;
    f->mask.vlan_id = mask;
}

void flow_set_masked_mpls_label(struct flow *f, uint32_t mpls_label, uint32_t mask)
{
    f->key.mpls_label = mpls_label & mask;
    f->mask.mpls_label = mask;
}

void flow_set_masked_ipv4_dst(struct flow *f, uint32_t ipv4_dst, uint32_t mask)
{
    f->key.ipv4_dst = ipv4_dst & mask;
    f->mask.ipv4_dst = mask;
}

void flow_set_masked_ipv4_src(struct flow *f, uint32_t ipv4_src, uint32_t mask)
{
    f->key.ipv4_src = ipv4_src & mask;
    f->mask.ipv4_src = mask;
}

void flow_set_masked_arp_spa(struct flow *f, uint32_t arp_spa, uint32_t mask)
{
    f->key.arp_spa = arp_spa & mask;
    f->mask.arp_spa = mask;
}

void flow_set_masked_arp_tpa(struct flow *f, uint32_t arp_tpa, uint32_t mask)
{
    f->key.arp_tpa = arp_tpa & mask;
    f->mask.arp_tpa = mask;
}

void flow_set_masked_arp_sha(struct flow *f, uint8_t arp_sha[6], uint8_t mask[6])
{

    memcpy(f->mask.arp_sha, mask, 6);
    memcpy(f->key.arp_sha, arp_sha, 6);
    apply_mask_eth_addr(f->key.arp_sha, mask);
}

void flow_set_masked_arp_tha(struct flow *f, uint8_t arp_tha[6], uint8_t mask[6])
{
    memcpy(f->mask.arp_tha, mask, 6);
    memcpy(f->key.arp_tha, arp_tha, 6);
    apply_mask_eth_addr(f->key.arp_tha, mask);
}

void flow_set_masked_ipv6_dst(struct flow *f, uint8_t ipv6_dst[16], uint8_t mask[16])
{
    memcpy(f->mask.ipv6_dst, mask, 16);
    memcpy(f->mask.ipv6_src, ipv6_dst, 16);
    apply_mask_ipv6(f->key.ipv6_dst, mask);
}

void flow_set_masked_ipv6_src(struct flow *f, uint8_t ipv6_src[16], uint8_t mask[16])
{
    memcpy(f->mask.ipv6_src, mask, 16);
    memcpy(f->mask.ipv6_src, ipv6_src, 16);
    apply_mask_ipv6(f->key.ipv6_src, mask);
}

void flow_set_masked_ipv6_nd_target(struct flow *f, uint8_t ipv6_nd_target[16], uint8_t mask[16])
{
    memcpy(f->mask.ipv6_nd_target, mask, 16);
    memcpy(f->key.ipv6_nd_target, ipv6_nd_target, 16);
    apply_mask_ipv6(f->key.ipv6_nd_target, mask);
}

void flow_set_masked_ipv6_nd_sll(struct flow *f, uint8_t ipv6_nd_sll[6], uint8_t mask[6])
{
    memcpy(f->mask.ipv6_nd_sll, mask, 6);
    memcpy(f->key.ipv6_nd_sll, ipv6_nd_sll, 6);
    apply_mask_eth_addr(f->key.ipv6_nd_sll, mask);
}

void flow_set_masked_ipv6_nd_tll(struct flow *f, uint8_t ipv6_nd_tll[6], uint8_t mask[6])
{
    memcpy(f->mask.ipv6_nd_tll, mask, 6);
    memcpy(f->key.ipv6_nd_tll, ipv6_nd_tll, 6);
    apply_mask_eth_addr(f->key.ipv6_nd_tll, mask);
}

void apply_all_mask(struct ofl_flow_key *key, struct ofl_flow_key *mask)
{
    key->in_port &= mask->in_port;
    key->metadata &= mask->metadata;
    key->tunnel_id &= mask->tunnel_id;
    key->eth_type &= mask->eth_type;
    apply_mask_eth_addr(key->eth_dst, mask->eth_dst);
    apply_mask_eth_addr(key->eth_src, mask->eth_src);
    key->vlan_id &= mask->vlan_id;
    key->vlan_pcp &= mask->vlan_pcp;
    key->mpls_label &= mask->mpls_label;
    key->mpls_tc &= mask->mpls_tc;
    key->mpls_bos &= mask->mpls_bos;
    key->ip_dscp &= mask->ip_dscp;
    key->ip_ecn &= mask->ip_ecn;
    key->ip_proto &= mask->ip_proto;
    key->ipv4_dst &= mask->ipv4_dst;
    key->ipv4_src &= mask->ipv4_src;
    key->tcp_dst &= mask->tcp_dst;
    key->tcp_src &= mask->tcp_src;
    key->sctp_dst &= mask->sctp_dst;
    key->sctp_src &= mask->sctp_src;
    key->udp_dst &= mask->udp_dst;
    key->udp_src &= mask->udp_src;
    key->arp_op &= mask->arp_op;
    key->icmpv4_type &= mask->icmpv4_type;
    key->icmpv4_code &= mask->icmpv4_code;
    key->arp_spa &= mask->arp_spa;
    key->arp_tpa &= mask->arp_tpa;
    apply_mask_eth_addr(key->arp_sha, mask->arp_sha);
    apply_mask_eth_addr(key->arp_tha, mask->arp_tha);
    apply_mask_ipv6(key->ipv6_dst, mask->ipv6_dst);
    apply_mask_ipv6(key->ipv6_src, mask->ipv6_src);
    key->ipv6_flabel &= mask->ipv6_flabel;
    key->icmpv6_type &= mask->icmpv6_type;
    key->icmpv6_code &= mask->icmpv6_code;
    apply_mask_ipv6(key->ipv6_nd_target, mask->ipv6_nd_target);
    apply_mask_eth_addr(key->ipv6_nd_sll, mask->ipv6_nd_sll);
    apply_mask_eth_addr(key->ipv6_nd_tll, mask->ipv6_nd_tll);
}
