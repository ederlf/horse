#ifndef PACKET_H
#define PACKET_H 1

#include <stdint.h>
#include <stddef.h>

#define ETH_LEN 6
#define IPV6_LEN 16

#define ETH_TYPE_IP            0x0800
#define ETH_TYPE_IPV6          0x86dd
#define ETH_TYPE_ARP           0x0806
#define ETH_TYPE_VLAN          0x8100
#define ETH_TYPE_LLDP          0x88cc
#define ETH_TYPE_VLAN_QinQ     0x88a8
#define ETH_TYPE_MPLS          0x8847
#define ETH_TYPE_MPLS_MCAST    0x8848

#define ARP_HW_TYPE_ETH 1
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

#define VLAN_VID_MASK 0x0fff
#define VLAN_VID_SHIFT 0
#define VLAN_PCP_MASK 0xe000
#define VLAN_PCP_SHIFT 13
#define VLAN_PCP_BITMASK 0x0007 /* the least 3-bit is valid */
#define VLAN_VID_MAX 4095
#define VLAN_PCP_MAX 7

#define MPLS_TTL_MASK 0x000000ff
#define MPLS_TTL_SHIFT 0
#define MPLS_S_MASK 0x00000100
#define MPLS_S_SHIFT 8
#define MPLS_TC_MASK 0x00000e00
#define MPLS_TC_SHIFT 9
#define MPLS_LABEL_MASK 0xfffff000
#define MPLS_LABEL_SHIFT 12

#define IP_ECN_MASK 0x03
#define IP_DSCP_MASK 0xfc

#define IP_DONT_FRAGMENT  0x4000 /* Don't fragment. */
#define IP_MORE_FRAGMENTS 0x2000 /* More fragments. */
#define IP_FRAG_OFF_MASK  0x1fff /* Fragment offset. */
#define IP_IS_FRAGMENT(ip_frag_off) \
        ((ip_frag_off) & htons(IP_MORE_FRAGMENTS | IP_FRAG_OFF_MASK))

#define IP_PROTO_ICMPV4 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17
#define IP_PROTO_ICMPV6 58

#define ICMPV6_NEIGH_SOL 135
#define ICMPV6_NEIGH_ADV 136   


struct eth_header {
    uint8_t eth_dst[ETH_LEN];
    uint8_t eth_src[ETH_LEN];
    uint16_t eth_type;
} __attribute__((packed));

struct vlan {
    uint16_t ethertype;
    uint16_t tag;
}__attribute__((packed));;

struct mpls {
    uint32_t fields; 
}__attribute__((packed));;

struct ip_header {
    uint8_t ip_ihl_ver;
    uint8_t ip_tos;
    uint16_t ip_tot_len;
    uint16_t ip_id;
    uint16_t ip_frag_off;
    uint8_t ip_ttl;
    uint8_t ip_proto;
    uint16_t ip_csum;
    uint32_t ip_src;
    uint32_t ip_dst;
}__attribute__((packed));;

struct ipv6_header {
    uint32_t ipv6_ver_tc_fl;
    uint16_t ipv6_pay_len;
    uint8_t  ipv6_next_hd;
        uint8_t ipv6_hop_limit;
    uint8_t ipv6_src[IPV6_LEN];
    uint8_t  ipv6_dst[IPV6_LEN];
}__attribute__((packed));;

struct ipv6_nd_header{
    uint32_t reserved;
    uint8_t target_addr[IPV6_LEN];
}__attribute__((packed));;

struct ipv6_nd_options_hd{
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

#define TCP_HEADER_LEN 20
struct tcp_header {
    uint16_t tcp_src;
    uint16_t tcp_dst;
    uint32_t tcp_seq;
    uint32_t tcp_ack;
    uint16_t tcp_ctl;
    uint16_t tcp_winsz;
    uint16_t tcp_csum;
    uint16_t tcp_urg;
}__attribute__((packed));

#define UDP_HEADER_LEN 8
struct udp_header {
    uint16_t udp_src;
    uint16_t udp_dst;
    uint16_t udp_len;
    uint16_t udp_csum;
}__attribute__((packed));

enum ping_types {
    ECHO_REPLY = 0,
    DST_UNREACHABLE = 3,
    ECHO = 8,
};

#define ICMP_HEADER_LEN 4
struct icmp_header {
    uint8_t icmp_type;
    uint8_t icmp_code;
    uint16_t icmp_csum;
}__attribute__((packed));

struct icmp_echo_reply {
    uint16_t identifier;
    uint16_t seq_number;
    uint8_t data[48];
}__attribute__((packed));

struct arp_eth_header {
    /* Generic members. */
    uint16_t arp_hrd;           /* Hardware type. */
    uint16_t arp_pro;           /* Protocol type. */
    uint8_t arp_hln;            /* Hardware address length. */
    uint8_t arp_pln;            /* Protocol address length. */
    uint16_t arp_op;            /* Opcode. */

    /* Ethernet+IPv4 specific members. */
    uint8_t arp_sha[ETH_LEN]; /* Sender hardware address. */
    uint32_t arp_spa;           /* Sender protocol address. */
    uint8_t arp_tha[ETH_LEN]; /* Target hardware address. */
    uint32_t arp_tpa;           /* Target protocol address. */
} __attribute__((packed));

#endif