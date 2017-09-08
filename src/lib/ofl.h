#ifndef OFL_H
#define OFL_H 1

struct ofl_flow_key {
    uint32_t in_port;
    uint32_t in_phy_port;
    uint64_t metadata;
    uint64_t tunnel_id;
    uint8_t  eth_dst[ETH_LEN];
    uint8_t  eth_src[ETH_LEN];
    uint16_t eth_type;
    uint16_t vlan_id;
    uint8_t  vlan_pcp;
    uint8_t  ip_dscp;
    uint8_t  ip_ecn;
    uint8_t  ip_proto;
    uint32_t ipv4_src;
    uint32_t ipv4_dst;
    uint16_t tcp_src;
    uint16_t tcp_dst;
    uint16_t udp_src;
    uint16_t udp_dst;
    uint16_t sctp_src;
    uint16_t sctp_dst;
    uint8_t  icmpv4_type;
    uint8_t  icmpv4_code;
    uint16_t arp_op;
    uint32_t arp_spa;
    uint32_t arp_tpa;
    uint8_t  arp_sha[ETH_LEN];
    uint8_t  arp_tha[ETH_LEN];
    uint8_t  ipv6_src[IPV6_LEN];
    uint8_t  ipv6_dst[IPV6_LEN];
    uint32_t ipv6_flabel;
    uint8_t  icmpv6_type;
    uint8_t  icmpv6_code;
    uint8_t  ipv6_nd_target[IPV6_LEN];
    uint8_t  ipv6_nd_sll[ETH_LEN];
    uint8_t  ipv6_nd_tll[ETH_LEN];
    uint32_t mpls_label;
    uint8_t  mpls_tc;
    uint8_t  mpls_bos;
};

struct ofl_flow_stats_req {
    uint8_t table_id;         /* ID of table to read (from ofp_table_stats),
                                 OFPTT_ALL for all tables. */
    uint32_t out_port;        /* Require matching entries to include this
                                 as an output port.  A value of OFPP_ANY
                                 indicates no restriction. */
    uint32_t out_group;       /* Require matching entries to include this
                                 as an output group.  A value of OFPG_ANY
                                 indicates no restriction. */
    uint64_t cookie;          /* Require matching entries to contain this
                                 cookie value */
    uint64_t cookie_mask;     /* Mask used to restrict the cookie bits that
                                 must match. A value of 0 indicates
                                 no restriction. */
    struct ofl_flow_key match;   /* Fields to match. Variable size. */

    struct ofl_flow_key mask;   /* Masked fields. Variable size. */
};

#endif