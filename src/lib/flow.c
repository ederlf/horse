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

/* Allocates a new flow and sets all fields to 0 */
struct flow* 
flow_new(void)
{
    struct flow *f = xmalloc(sizeof(struct flow));
    f->priority = 0;
    f->cookie = 0;
    memset(&f->key, 0x0, sizeof(struct flow_key));
    memset(&f->mask, 0x0, sizeof(struct flow_key));
    return f;
}

void 
flow_destroy(struct flow *f)
{
    free(f);
}

bool 
flow_key_cmp(struct flow_key *a, struct flow_key *b)
{
    return  (a->in_port == b->in_port) &&
            (a->metadata == b->metadata) &&
            (a->tunnel_id == b->tunnel_id) &&
            (a->eth_type == b->eth_type) &&
            (memcmp(a->eth_dst, b->eth_dst, 6) == 0) &&
            (memcmp(a->eth_src, b->eth_src, 6) == 0) &&
            (a->vlan_id == b->vlan_id) &&
            (a->vlan_pcp == b->vlan_pcp) &&
            (a->mpls_label == b->mpls_label) &&
            (a->mpls_bos == b->mpls_bos) &&
            (a->mpls_tc == b->mpls_tc) &&
            (a->ip_dscp == b->ip_dscp) &&
            (a->ip_ecn == b->ip_ecn) &&
            (a->ip_proto == b->ip_proto) &&
            (a->ipv4_dst == b->ipv4_dst) &&
            (a->ipv4_src == b->ipv4_src) &&
            (a->tp_dst == b->tp_dst) &&
            (a->tp_src == b->tp_src) &&
            (a->arp_op == b->arp_op) &&
            (a->arp_spa == b->arp_spa) &&
            (a->arp_tpa == b->arp_tpa) &&
            (memcmp(a->arp_sha, b->arp_sha, 6) == 0) &&
            (memcmp(a->arp_tha, b->arp_tha, 6) == 0) &&
            (memcmp(a->ipv6_dst, b->ipv6_dst, 16) == 0) &&
            (memcmp(a->ipv6_src, b->ipv6_src, 16) == 0) &&
            (memcmp(a->ipv6_nd_target, b->ipv6_nd_target, 16) == 0) &&
            (memcmp(a->ipv6_nd_sll, b->ipv6_nd_sll, 6) == 0) &&
            (memcmp(a->ipv6_nd_tll, b->ipv6_nd_tll, 6) == 0);
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
set_tp_dst(struct flow *f, uint16_t tp_dst)
{
    f->key.tp_dst = tp_dst;
    f->mask.tp_dst = ALL_UINT16_MASK;
}

void 
set_tp_src(struct flow *f, uint16_t tp_src)
{
    f->key.tp_src = tp_src;
    f->mask.tp_src = ALL_UINT16_MASK;
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

void set_masked_metadata(struct flow *f, uint64_t metadata, uint64_t mask)
{
    f->key.metadata = metadata & mask;
    f->mask.in_port = mask;
}

void set_masked_tunnel_id(struct flow *f, uint64_t tunnel_id, uint64_t mask)
{
    f->key.tunnel_id = tunnel_id & mask;
    f->mask.tunnel_id = mask;
}

void set_masked_eth_dst(struct flow *f, uint8_t eth_dst[6], uint8_t mask[6])
{
    int i;
    for (i = 0; i < 6; ++i){
        f->key.eth_dst[i] = eth_dst[i] & mask[i];
        f->mask.eth_dst[i] = mask[i];
    }
}

void set_masked_eth_src(struct flow *f, uint8_t eth_src[6], uint8_t mask[6])
{
    int i;
    for (i = 0; i < 6; ++i){
        f->key.eth_src[i] = eth_src[i] & mask[i];
        f->mask.eth_src[i] = mask[i];
    }
}

void set_masked_vlan_id(struct flow *f, uint16_t vlan_id, uint16_t mask)
{
    f->key.vlan_id = vlan_id & mask;
    f->mask.vlan_id = mask;
}

void set_masked_mpls_label(struct flow *f, uint32_t mpls_label, uint32_t mask)
{
    f->key.mpls_label = mpls_label & mask;
    f->mask.mpls_label = mask;
}

void set_masked_ipv4_dst(struct flow *f, uint32_t ipv4_dst, uint32_t mask)
{
    f->key.ipv4_dst = ipv4_dst & mask;
    f->mask.ipv4_dst = mask;
}

void set_masked_ipv4_src(struct flow *f, uint32_t ipv4_src, uint32_t mask)
{
    f->key.ipv4_src = ipv4_src & mask;
    f->mask.ipv4_src = mask;
}

void set_masked_arp_spa(struct flow *f, uint32_t arp_spa, uint32_t mask)
{
    f->key.arp_spa = arp_spa & mask;
    f->mask.arp_spa = mask;
}

void set_masked_arp_tpa(struct flow *f, uint32_t arp_tpa, uint32_t mask)
{
    f->key.arp_tpa = arp_tpa & mask;
    f->mask.arp_tpa = mask;
}

void set_masked_arp_sha(struct flow *f, uint8_t arp_sha[6], uint8_t mask[6])
{
    int i;
    for (i = 0; i < 6; ++i){
        f->key.arp_sha[i] = arp_sha[i] & mask[i];
        f->mask.arp_sha[i] = mask[i];
    }
}

void set_masked_arp_tha(struct flow *f, uint8_t arp_tha[6], uint8_t mask[6])
{
    int i;
    for (i = 0; i < 6; ++i){
        f->key.arp_tha[i] = arp_tha[i] & mask[i];
        f->mask.arp_tha[i] = mask[i];
    }
}

void set_masked_ipv6_dst(struct flow *f, uint8_t ipv6_dst[16], uint8_t mask[16])
{
    int i;
    for (i = 0; i < 16; ++i){
        f->key.ipv6_dst[i] = ipv6_dst[i] & mask[i];
        f->mask.ipv6_dst[i] = mask[i];
    }
}

void set_masked_ipv6_src(struct flow *f, uint8_t ipv6_src[16], uint8_t mask[16])
{
    int i;
    for (i = 0; i < 16; ++i){
        f->key.ipv6_src[i] = ipv6_src[i] & mask[i];
        f->mask.ipv6_src[i] = mask[i];
    }
}

void set_masked_ipv6_nd_target(struct flow *f, uint8_t ipv6_nd_target[16], uint8_t mask[16])
{
    int i;
    for (i = 0; i < 16; ++i){
        f->key.ipv6_nd_target[i] = ipv6_nd_target[i] & mask[i];
        f->mask.ipv6_nd_target[i] = mask[i];
    }
}

void set_masked_ipv6_nd_sll(struct flow *f, uint8_t ipv6_nd_sll[6], uint8_t mask[6])
{
    int i;
    for (i = 0; i < 6; ++i){
        f->key.ipv6_nd_sll[i] = ipv6_nd_sll[i] & mask[i];
        f->mask.ipv6_nd_sll[i] = mask[i];
    }
}

void set_masked_ipv6_nd_tll(struct flow *f, uint8_t ipv6_nd_tll[6], uint8_t mask[6])
{
    int i;
    for (i = 0; i < 6; ++i){
        f->key.ipv6_nd_tll[i] = ipv6_nd_tll[i] & mask[i];
        f->mask.ipv6_nd_tll[i] = mask[i];
    }
}

void 
apply_all_mask(struct flow *flow, struct flow_key *mask)
{
    flow->key.in_port &= mask->in_port;
    set_masked_metadata(flow, flow->key.metadata, mask->metadata);
    set_masked_tunnel_id(flow, flow->key.tunnel_id, mask->tunnel_id);
    flow->key.eth_type &= flow->key.eth_type;
    set_masked_eth_dst(flow, flow->key.eth_dst, mask->eth_dst);
    set_masked_eth_src(flow, flow->key.eth_src, mask->eth_src);
    set_masked_vlan_id(flow, flow->key.vlan_id, mask->vlan_id);
    flow->key.vlan_pcp &= mask->vlan_pcp;
    set_masked_mpls_label(flow, flow->key.mpls_label, mask->mpls_label);
    flow->key.mpls_tc &= mask->mpls_tc;
    flow->key.mpls_bos &= mask->mpls_bos;
    flow->key.ip_dscp &= mask->ip_dscp;
    flow->key.ip_ecn &= mask->ip_ecn;
    flow->key.ip_proto &= mask->ip_proto;
    set_masked_ipv4_dst(flow, flow->key.ipv4_dst, mask->ipv4_dst);
    set_masked_ipv4_src(flow, flow->key.ipv4_src, mask->ipv4_src);
    flow->key.tp_dst &= mask->tp_dst;
    flow->key.tp_src &= mask->tp_src;
    flow->key.arp_op &= mask->arp_op;
    set_masked_arp_spa(flow, flow->key.arp_spa, mask->arp_spa);
    set_masked_arp_tpa(flow, flow->key.arp_tpa, mask->arp_tpa);
    set_masked_arp_sha(flow, flow->key.arp_sha, mask->arp_sha);
    set_masked_arp_tha(flow, flow->key.arp_tha, mask->arp_tha);
    set_masked_ipv6_dst(flow, flow->key.ipv6_dst, mask->ipv6_dst);
    set_masked_ipv6_src(flow, flow->key.ipv6_src, mask->ipv6_src);
    set_masked_ipv6_nd_target(flow, flow->key.ipv6_nd_target, mask->ipv6_nd_target);
    set_masked_ipv6_nd_sll(flow, flow->key.ipv6_nd_sll, mask->ipv6_nd_sll);
    set_masked_ipv6_nd_tll(flow, flow->key.ipv6_nd_tll, mask->ipv6_nd_tll);
}