#include "flow.h"
#include <stdlib.h>
#include <string.h>

#define ALL_UINT8_MASK 0xff
#define ALL_UINT16_MASK 0xffff
#define ALL_UINT32_MASK 0xffffffff
#define ALL_UINT64_MASK 0xffffffffffffffff

/* Allocates a new flow and sets all fields to 0 */
struct flow* 
new_flow(void)
{
    struct flow *f = malloc(sizeof(struct flow));
    f->priority = 0;
    memset(&f->key, 0x0, sizeof(struct flow_key));
    memset(&f->mask, 0x0, sizeof(struct flow_key));
    return f;
}

void 
set_in_port(struct flow* f, uint32_t in_port)
{
    f->key.in_port = in_port;
    f->mask.in_port = ALL_UINT32_MASK;
}

void 
set_metadata(struct flow* f, uint64_t metadata)
{
    f->key.metadata = metadata;
    f->mask.metadata = ALL_UINT64_MASK;
}

void 
set_tunnel_id(struct flow* f, uint64_t tunnel_id)
{
    f->key.tunnel_id = tunnel_id;
    f->mask.tunnel_id = ALL_UINT64_MASK;
}

void 
set_eth_dst(struct flow* f, uint8_t eth_dst[6])
{
    memcpy(f->key.eth_dst, eth_dst, 6);
    memset(f->mask.eth_dst, 0xff, 6);
}

void
set_eth_src(struct flow* f, uint8_t eth_src[6])
{
    memcpy(f->key.eth_src, eth_src, 6);
    memset(f->mask.eth_src, 0xff, 6);
}

void 
set_vlan_id(struct flow* f, uint16_t vlan_id)
{
    f->key.vlan_id = vlan_id;
    f->mask.vlan_id = ALL_UINT16_MASK;
}

void 
set_vlan_pcp(struct flow* f, uint8_t vlan_pcp)
{
    f->key.vlan_pcp = vlan_pcp;
    f->mask.vlan_pcp = ALL_UINT8_MASK;
}

void 
set_mpls_label(struct flow* f, uint32_t mpls_label)
{
    f->key.mpls_label = mpls_label;
    f->mask.mpls_label = ALL_UINT32_MASK;
}

void 
set_mpls_tc(struct flow* f, uint8_t mpls_tc)
{
    f->key.mpls_tc = mpls_tc;
    f->mask.mpls_tc = ALL_UINT8_MASK;
}

void 
set_mpls_bos(struct flow* f, uint8_t mpls_bos)
{
    f->key.mpls_bos = mpls_bos;
    f->mask.mpls_bos = ALL_UINT8_MASK;
}

void 
set_ip_dscp(struct flow* f, uint8_t ip_dscp)
{
    f->key.ip_dscp = ip_dscp;
    f->mask.ip_dscp = ALL_UINT8_MASK;
}

void 
set_ip_ecn(struct flow* f, uint8_t ip_ecn)
{
    f->key.ip_ecn = ip_ecn;
    f->mask.ip_ecn = ALL_UINT8_MASK;
}

void 
set_ip_proto(struct flow* f, uint8_t ip_proto)
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

void set_masked_metadata(struct flow* f, uint64_t metadata, uint64_t mask)
{
    f->key.metadata = metadata && mask;
    f->mask.in_port = mask;
}

void set_masked_tunnel_id(struct flow* f, uint64_t tunnel_id, uint64_t mask)
{
    f->key.tunnel_id = tunnel_id && mask;
    f->mask.tunnel_id = mask;
}

// void set_masked_eth_dst(struct flow* f, uint8_t eth_dst[6], uint8_t mask[6])
// {


// }

// void set_masked_eth_src(struct flow* f, uint8_t eth_src[6], uint8_t mask[6])
// {

// }

void set_masked_vlan_id(struct flow* f, uint16_t vlan_id, uint16_t mask)
{
    f->key.vlan_id = vlan_id && mask;
    f->mask.vlan_id = mask;
}

void set_masked_mpls_label(struct flow* f, uint32_t mpls_label, uint32_t mask)
{
    f->key.mpls_label = mpls_label && mask;
    f->mask.mpls_label = mask;
}

void set_masked_ipv4_dst(struct flow *f, uint32_t ipv4_dst, uint32_t mask)
{
    f->key.ipv4_dst = ipv4_dst && mask;
    f->mask.ipv4_dst = mask;
}

void set_masked_ipv4_src(struct flow *f, uint32_t ipv4_src, uint32_t mask)
{
    f->key.ipv4_src = ipv4_src && mask;
    f->mask.ipv4_src = mask;
}

void set_masked_arp_spa(struct flow *f, uint32_t arp_spa, uint32_t mask)
{
    f->key.arp_spa = arp_spa && mask;
    f->mask.arp_spa = mask;
}

void set_masked_arp_tpa(struct flow *f, uint32_t arp_tpa, uint32_t mask)
{
    f->key.arp_tpa = arp_tpa && mask;
    f->mask.arp_tpa = mask;
}

// void set_masked_arp_sha(struct flow *f, uint8_t arp_sha[6], uint8_t mask[6])
// {

// }

// void set_masked_arp_tha(struct flow *f, uint8_t arp_tha[6], uint8_t mask[6])
// {

// }

// void set_masked_ipv6_dst(struct flow *f, uint8_t ipv6_dst[16], uint8_t mask[16])
// {

// }
// void set_masked_ipv6_src(struct flow *f, uint8_t ipv6_src[16], uint8_t mask[16])
// {

// }

// void set_masked_ipv6_nd_target(struct flow *f, uint8_t ipv6_nd_target[16], uint8_t mask[16])
// {


// }

// void set_masked_ipv6_nd_sll(struct flow *f, uint8_t ipv6_nd_sll[6], uint8_t mask[6])
// {

// }

// void set_masked_ipv6_nd_tll(struct flow *f, uint8_t ipv6_nd_tll[6], uint8_t mask[6])
// {

// }