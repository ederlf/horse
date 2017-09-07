#include "of_pack.h"
#include "sim_event.h"
#include "util.h"
#include "packets.h"
#include <loci/loci.h>

// #define ETH_TYPE_IP            0x0800
// #define ETH_TYPE_IPV6          0x86dd
// #define ETH_TYPE_ARP           0x0806
// #define ETH_TYPE_VLAN          0x8100
// #define ETH_TYPE_VLAN_QinQ     0x88a8
// #define ETH_TYPE_MPLS          0x8847
// #define ETH_TYPE_MPLS_MCAST    0x8848

static void netflow_to_match(struct netflow *flow, of_match_t *match)
{
    struct ofl_flow_key m =  flow->match;
    memset(match, 0x0, sizeof(of_match_t));
    match->version = OF_VERSION_1_3;
    match->fields.in_port = m.in_port;
    OF_MATCH_MASK_IN_PORT_EXACT_SET(match);    
}

uint8_t *of_packet_in(struct netflow *f, size_t *len)
{
    uint32_t pkt_len;
    uint8_t *buf = NULL;
    of_packet_in_t *of_packet_in;
    of_match_t     match;
    of_octets_t of_octets;

    of_octets.data = xmalloc(MAX_PACKET_IN_DATA);
    netflow_to_match(f, &match);
    of_packet_in = of_packet_in_new(OF_VERSION_1_3);
    of_packet_in_buffer_id_set(of_packet_in, f->metadata.buffer_id);
    of_packet_in_reason_set(of_packet_in, f->metadata.reason);
    of_packet_in_table_id_set(of_packet_in, f->metadata.table_id);
    of_packet_in_cookie_set(of_packet_in, f->metadata.cookie);
    if (of_packet_in_match_set(of_packet_in, &match) != OF_ERROR_NONE){
        return NULL;
    }
    pkt_len = netflow_to_pkt(f, of_octets.data);
    // printf("Needs to send packet_in %d\n", of_packet_in->length);
    of_octets.bytes = pkt_len;
    if ((of_packet_in_data_set(of_packet_in, &of_octets)) != OF_ERROR_NONE) {
        printf("Failed to write packet data to packet-in message\n");
        of_packet_in_delete(of_packet_in);
        return NULL;
    }
    of_packet_in_total_len_set(of_packet_in, pkt_len);
    *len = of_packet_in->length;
    of_object_wire_buffer_steal((of_object_t*) of_packet_in, &buf);
    // buf = (uint8_t*) WBUF_BUF(OF_OBJECT_TO_WBUF(of_packet_in));
    free(of_octets.data);
    of_packet_in_delete(of_packet_in);
    return buf;
}