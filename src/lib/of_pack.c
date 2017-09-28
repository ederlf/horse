#include "of_pack.h"
#include "sim_event.h"
#include "util.h"
#include "packets.h"
#include <uthash/utlist.h>

// #define ETH_TYPE_IP            0x0800
// #define ETH_TYPE_IPV6          0x86dd
// #define ETH_TYPE_ARP           0x0806
// #define ETH_TYPE_VLAN          0x8100
// #define ETH_TYPE_VLAN_QinQ     0x88a8
// #define ETH_TYPE_MPLS          0x8847
// #define ETH_TYPE_MPLS_MCAST    0x8848

static
void netflow_to_match(struct netflow *flow, of_match_t *match)
{
    struct ofl_flow_key m =  flow->match;
    memset(match, 0x0, sizeof(of_match_t));
    match->version = OF_VERSION_1_3;
    match->fields.in_port = m.in_port;
    OF_MATCH_MASK_IN_PORT_EXACT_SET(match);
}

void
pack_match_fields(struct ofl_flow_key *key, of_match_fields_t *fields)
{
    fields->in_port = key->in_port;
    fields->in_phy_port = key->in_phy_port;
    fields->metadata = key->metadata;
    fields->tunnel_id = key->tunnel_id;
    memcpy(fields->eth_dst.addr, key->eth_dst, ETH_LEN);
    memcpy(fields->eth_src.addr, key->eth_src, ETH_LEN);
    fields->eth_type = key-> eth_type;
    fields->vlan_vid = key->vlan_id;
    fields->vlan_pcp = key-> vlan_pcp;
    fields->ip_dscp = key->ip_dscp;
    fields->ip_ecn = key->ip_ecn;
    fields->ip_proto = key->ip_proto;
    fields->ipv4_src = key->ipv4_src;
    fields->ipv4_dst = key->ipv4_dst;
    fields->tcp_src = key->tcp_src;
    fields->tcp_dst = key->tcp_dst;
    fields->udp_src = key->udp_src;
    fields->udp_dst = key->udp_dst;
    fields->sctp_src = key->sctp_src;
    fields->sctp_dst = key->sctp_dst;
    fields->icmpv4_type = key->icmpv4_type;
    fields->icmpv4_code = key->icmpv4_code;
    fields->arp_op = key->arp_op;
    fields->arp_spa = key->arp_spa;
    fields->arp_tpa = key->arp_tpa;
    memcpy(fields->arp_sha.addr, key->arp_sha, ETH_LEN);
    memcpy(fields->arp_tha.addr, key->arp_tha, ETH_LEN);
    memcpy(fields->ipv6_src.addr, key->ipv6_src, IPV6_LEN);
    memcpy(fields->ipv6_dst.addr, key->ipv6_dst, IPV6_LEN);
    fields->ipv6_flabel = key->ipv6_flabel;
    fields->icmpv6_type = key->icmpv6_type;
    fields->icmpv6_code = fields->icmpv6_code;
    memcpy(fields->ipv6_nd_target.addr, key->ipv6_nd_target, IPV6_LEN);
    memcpy(fields->ipv6_nd_sll.addr, key->ipv6_nd_sll, ETH_LEN);
    memcpy(fields->ipv6_nd_tll.addr, key->ipv6_nd_tll, ETH_LEN);
    fields->mpls_label = key->mpls_label;
    fields->mpls_tc = key->mpls_tc;
    fields->mpls_bos = key->mpls_bos;
}

of_object_t * pack_set_field(struct action *act)
{
    of_action_set_field_t *a = of_action_set_field_new(OF_VERSION_1_3);
    switch (act->set.field) {
    case SET_METADATA: {
        of_oxm_metadata_t *oxm = of_oxm_metadata_new(OF_VERSION_1_3);
        of_oxm_metadata_value_set(oxm, act->set.u64_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_METADATA);
        }
        break;
    }
    case SET_TUNNEL_ID: {
        of_oxm_tunnel_id_t *oxm = of_oxm_tunnel_id_new(OF_VERSION_1_3);
        of_oxm_tunnel_id_value_set(oxm, act->set.u64_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_TUNNEL_ID);
        }
        break;
    }
    case SET_ETH_TYPE: {
        of_oxm_eth_type_t *oxm = of_oxm_eth_type_new(OF_VERSION_1_3);
        of_oxm_eth_type_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ETH_TYPE);
        }
        break;
    }
    case SET_ETH_DST: {
        of_mac_addr_t value;
        memcpy(value.addr, act->set.eth_addr, ETH_LEN);
        of_oxm_eth_dst_t *oxm = of_oxm_eth_dst_new(OF_VERSION_1_3);
        of_oxm_eth_dst_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ETH_DST);
        }
        break;
    }
    case SET_ETH_SRC: {
        of_mac_addr_t value;
        memcpy(value.addr, act->set.eth_addr, ETH_LEN);
        of_oxm_eth_src_t *oxm = of_oxm_eth_src_new(OF_VERSION_1_3);
        of_oxm_eth_src_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ETH_SRC);
        }
        break;
    }
    case SET_VLAN_ID: {
        of_oxm_vlan_vid_t *oxm = of_oxm_vlan_vid_new(OF_VERSION_1_3);
        of_oxm_vlan_vid_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_VLAN_ID);
        }
        break;
    }
    case SET_VLAN_PCP: {
        of_oxm_vlan_pcp_t *oxm = of_oxm_vlan_pcp_new(OF_VERSION_1_3);
        of_oxm_vlan_pcp_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_VLAN_PCP);
        }
        break;
    }
    case SET_MPLS_LABEL: {
        of_oxm_mpls_label_t *oxm = of_oxm_mpls_label_new(OF_VERSION_1_3);
        of_oxm_mpls_label_value_set(oxm, act->set.u32_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_MPLS_LABEL);
        }
        break;
    }
    case SET_MPLS_TC: {
        of_oxm_mpls_tc_t *oxm = of_oxm_mpls_tc_new(OF_VERSION_1_3);
        of_oxm_mpls_tc_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_MPLS_TC);
        }
        break;
    }
    case SET_MPLS_BOS: {
        of_oxm_mpls_bos_t *oxm = of_oxm_mpls_bos_new(OF_VERSION_1_3);
        of_oxm_mpls_bos_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_MPLS_BOS);
        }
        break;
    }
    case SET_IP_DSCP: {
        of_oxm_ip_dscp_t *oxm = of_oxm_ip_dscp_new(OF_VERSION_1_3);
        of_oxm_ip_dscp_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IP_DSCP);
        }
        break;
    }
    case SET_IP_ECN: {
        of_oxm_ip_ecn_t *oxm = of_oxm_ip_ecn_new(OF_VERSION_1_3);
        of_oxm_ip_ecn_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IP_ECN);
        }
        break;
    }
    case SET_IP_PROTO: {
        of_oxm_ip_proto_t *oxm = of_oxm_ip_proto_new(OF_VERSION_1_3);
        of_oxm_ip_proto_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IP_PROTO);
        }
        break;
    }
    case SET_IPV4_DST: {
        of_oxm_ipv4_dst_t *oxm = of_oxm_ipv4_dst_new(OF_VERSION_1_3);
        of_oxm_ipv4_dst_value_set(oxm, act->set.u32_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV4_DST);
        }
        break;
    }
    case SET_IPV4_SRC: {
        of_oxm_ipv4_src_t *oxm = of_oxm_ipv4_src_new(OF_VERSION_1_3);
        of_oxm_ipv4_src_value_set(oxm, act->set.u32_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV4_SRC);
        }
        break;
    }
    case SET_ICMPV4_TYPE: {
        of_oxm_icmpv4_type_t *oxm = of_oxm_icmpv4_type_new(OF_VERSION_1_3);
        of_oxm_icmpv4_type_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ICMPV4_TYPE);
        }
        break;
    }
    case SET_ICMPV4_CODE: {
        of_oxm_icmpv4_code_t *oxm = of_oxm_icmpv4_code_new(OF_VERSION_1_3);
        of_oxm_icmpv4_code_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ICMPV4_CODE);
        }
        break;
    }
    case SET_TCP_DST: {
        of_oxm_tcp_dst_t *oxm = of_oxm_tcp_dst_new(OF_VERSION_1_3);
        of_oxm_tcp_dst_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_TCP_DST);
        }
        break;
    }
    case SET_TCP_SRC: {
        of_oxm_tcp_src_t *oxm = of_oxm_tcp_src_new(OF_VERSION_1_3);
        of_oxm_tcp_src_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_TCP_SRC);
        }
        break;
    }
    case SET_UDP_DST: {
        of_oxm_udp_dst_t *oxm = of_oxm_udp_dst_new(OF_VERSION_1_3);
        of_oxm_udp_dst_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_UDP_DST);
        }
        break;
    }
    case SET_UDP_SRC: {
        of_oxm_udp_src_t *oxm = of_oxm_udp_src_new(OF_VERSION_1_3);
        of_oxm_udp_src_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_UDP_SRC);
        }
        break;
    }
    case SET_SCTP_DST: {
        of_oxm_sctp_dst_t *oxm = of_oxm_sctp_dst_new(OF_VERSION_1_3);
        of_oxm_sctp_dst_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_SCTP_DST);
        }
        break;
    }
    case SET_SCTP_SRC: {
        of_oxm_sctp_src_t *oxm = of_oxm_sctp_src_new(OF_VERSION_1_3);
        of_oxm_sctp_src_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_SCTP_SRC);
        }
        break;
    }
    case SET_ARP_OP: {
        of_oxm_arp_op_t *oxm = of_oxm_arp_op_new(OF_VERSION_1_3);
        of_oxm_arp_op_value_set(oxm, act->set.u16_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ARP_OP);
        }
        break;
    }
    case SET_ARP_SPA: {
        of_oxm_arp_spa_t *oxm = of_oxm_arp_spa_new(OF_VERSION_1_3);
        of_oxm_arp_spa_value_set(oxm, act->set.u32_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ARP_SPA);
        }
        break;
    }
    case SET_ARP_TPA: {
        of_oxm_arp_tpa_t *oxm = of_oxm_arp_tpa_new(OF_VERSION_1_3);
        of_oxm_arp_tpa_value_set(oxm, act->set.u32_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ARP_TPA);
        }
        break;
    }
    case SET_ARP_SHA: {
        of_mac_addr_t value;
        memcpy(value.addr, act->set.eth_addr, ETH_LEN);
        of_oxm_arp_sha_t *oxm = of_oxm_arp_sha_new(OF_VERSION_1_3);
        of_oxm_arp_sha_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ARP_SHA);
        }
        break;
    }
    case SET_ARP_THA: {
        of_mac_addr_t value;
        memcpy(value.addr, act->set.eth_addr, ETH_LEN);
        of_oxm_arp_tha_t *oxm = of_oxm_arp_tha_new(OF_VERSION_1_3);
        of_oxm_arp_tha_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ARP_THA);
        }
        break;
    }
    case SET_IPV6_DST: {
        of_ipv6_t value;
        memcpy(value.addr, act->set.ipv6_addr, ETH_LEN);
        of_oxm_ipv6_dst_t *oxm = of_oxm_ipv6_dst_new(OF_VERSION_1_3);
        of_oxm_ipv6_dst_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV6_DST);
        }
        break;
    }
    case SET_IPV6_SRC: {
        of_ipv6_t value;
        memcpy(value.addr, act->set.ipv6_addr, ETH_LEN);
        of_oxm_ipv6_src_t *oxm = of_oxm_ipv6_src_new(OF_VERSION_1_3);
        of_oxm_ipv6_src_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV6_SRC);
        }
        break;
    }
    case SET_IPV6_FLABEL: {
        of_oxm_ipv6_flabel_t *oxm = of_oxm_ipv6_flabel_new(OF_VERSION_1_3);
        of_oxm_ipv6_flabel_value_set(oxm, act->set.u32_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV6_FLABEL);
        }
        break;
    }
    case SET_ICMPV6_TYPE: {
        of_oxm_icmpv6_type_t *oxm = of_oxm_icmpv6_type_new(OF_VERSION_1_3);
        of_oxm_icmpv6_type_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ICMPV6_TYPE);
        }
        break;
    }
    case SET_ICMPV6_CODE: {
        of_oxm_icmpv6_code_t *oxm = of_oxm_icmpv6_code_new(OF_VERSION_1_3);
        of_oxm_icmpv6_code_value_set(oxm, act->set.u8_field);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_ICMPV6_CODE);
        }
        break;
    }
    case SET_IPV6_ND_TARGET: {
        of_ipv6_t value;
        memcpy(value.addr, act->set.ipv6_addr, ETH_LEN);
        of_oxm_ipv6_nd_target_t *oxm = of_oxm_ipv6_nd_target_new(OF_VERSION_1_3);
        of_oxm_ipv6_nd_target_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV6_ND_TARGET);
        }
        break;
    }
    case SET_IPV6_ND_SLL: {
        of_mac_addr_t value;
        memcpy(value.addr, act->set.eth_addr, ETH_LEN);
        of_oxm_ipv6_nd_sll_t *oxm = of_oxm_ipv6_nd_sll_new(OF_VERSION_1_3);
        of_oxm_ipv6_nd_sll_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV6_ND_SLL);
        }
        break;
    }
    case SET_IPV6_ND_TLL: {
        of_mac_addr_t value;
        memcpy(value.addr, act->set.eth_addr, ETH_LEN);
        of_oxm_ipv6_nd_tll_t *oxm = of_oxm_ipv6_nd_tll_new(OF_VERSION_1_3);
        of_oxm_ipv6_nd_tll_value_set(oxm, value);
        if ( of_action_set_field_field_set(a, oxm) < 0 ) {
            fprintf(stderr, "%s->%d\n", "Failure to set field", SET_IPV6_ND_TLL);
        }
        break;
    }
    }
    return a;
}

of_object_t *pack_action (struct action *act)
{
    switch (act->type) {
    case ACT_OUTPUT: {
        of_action_output_t *a = of_action_output_new(OF_VERSION_1_3);
        of_action_output_port_set(a, act->out.port);
        /* Do we need max len? */
        of_action_output_max_len_set(a, 0);
        return  a;
    }
    case ACT_SET_FIELD: {
        return pack_set_field(act);
    }
    case ACT_POP_VLAN : {
        return of_action_pop_vlan_new(OF_VERSION_1_3);
    }
    case ACT_POP_MPLS : {
        of_action_pop_mpls_t *a = of_action_pop_mpls_new(OF_VERSION_1_3);
        of_action_pop_mpls_ethertype_set(a, act->pop_mpls.eth_type);
        return a;
    }
    case ACT_PUSH_MPLS : {
        of_action_push_mpls_t *a = of_action_push_mpls_new(OF_VERSION_1_3);
        of_action_push_mpls_ethertype_set(a, act->psh.eth_type);
        return a;
    }
    case ACT_PUSH_VLAN: {
        of_action_push_vlan_t *a = of_action_push_vlan_new(OF_VERSION_1_3);
        of_action_push_vlan_ethertype_set(a, act->psh.eth_type);
        return a;
    }
    case ACT_GROUP: {
        of_action_group_t *a = of_action_group_new(OF_VERSION_1_3);
        of_action_group_group_id_set(a, act->grp.group_id);
        return a;
        break;
    }
    /* Not supported yet */
    case ACT_METER : {
        break;
    }
    case ACT_COPY_TTL_INWARDS : {
        break;
    }
    case ACT_COPY_TTL_OUTWARDS: {
        break;
    }
    case ACT_DECREMENT_TTL: {
        break;
    }
    case ACT_SET_MPLS_TTL: {
        break;
    }
    case ACT_DECREMENT_MPLS_TTL: {
        break;
    }
    case ACT_QOS: {
        break;
    }
    }
    return NULL;
}

void
pack_action_list(struct action_list *al,  of_list_action_t *acts)
{
    struct action_list_elem *act_elem;
    LL_FOREACH(al->actions, act_elem) {
        of_object_t* act = pack_action(&act_elem->act);
        if (act != NULL) {
            of_list_action_append(acts, act);
            of_object_delete(act);
        }
    }
}

void
pack_action_set(struct action_set *as, of_list_action_t *acts)
{
    enum action_set_order type;
    /* Loop through the enum */
    for (type = ACT_METER; type <= ACT_OUTPUT; ++type) {
        struct action *act = action_set_action(as, type);
        if (act) {
            of_object_t* act_obj = pack_action(act);
            if (act_obj != NULL ) {
                of_list_action_append(acts, act_obj);
            }
        }
    }
}

void
pack_instructions(struct instruction_set *is, of_list_instruction_t *insts)
{

    if (instruction_is_active(is, INSTRUCTION_APPLY_ACTIONS)) {
        of_instruction_apply_actions_t *apply = of_instruction_apply_actions_new (OF_VERSION_1_3);
        of_list_action_t *acts = of_list_action_new(OF_VERSION_1_3);
        pack_action_list(&is->apply_act.actions, acts);
        
        if ( of_instruction_apply_actions_actions_set(apply, acts) < 0 ){
            fprintf(stderr, "%s\n", "Failure to set actions on apply actions");
        }
        
        of_list_instruction_append(insts, apply);
        of_list_action_delete(acts);
        of_instruction_apply_actions_delete(apply);
    }

    if (instruction_is_active(is, INSTRUCTION_CLEAR_ACTIONS)) {
        of_instruction_clear_actions_t *clear = of_instruction_clear_actions_new (OF_VERSION_1_3);
        of_list_instruction_append(insts, clear);
        of_instruction_clear_actions_delete(clear);
    }

    if (instruction_is_active(is, INSTRUCTION_WRITE_ACTIONS)) {
        of_instruction_write_actions_t *write = of_instruction_write_actions_new (OF_VERSION_1_3);
        of_list_action_t *acts = of_list_action_new(OF_VERSION_1_3);
        pack_action_set(&is->write_act.actions, acts);
        if (of_instruction_write_actions_actions_set(write, acts) < 0 ){
            fprintf(stderr, "%s\n", "Failure to set actions on write actions");
        }
        of_list_instruction_append(insts, write);
        of_list_action_delete(acts);
        of_instruction_write_actions_delete(write);
    }

    if (instruction_is_active(is, INSTRUCTION_WRITE_METADATA)) {
        of_instruction_write_metadata_t *wm = of_instruction_write_metadata_new(OF_VERSION_1_3);
        of_instruction_write_metadata_metadata_set(wm, is->write_meta.metadata);
        of_instruction_write_metadata_metadata_mask_set(wm, is->write_meta.metadata_mask);
        of_list_instruction_append(insts, wm);
        of_instruction_write_metadata_delete(wm);
    }

    if (instruction_is_active(is, INSTRUCTION_GOTO_TABLE)) {
        of_instruction_goto_table_t *gt = of_instruction_goto_table_new(OF_VERSION_1_3);
        of_instruction_goto_table_table_id_set(gt, is->gt_table.table_id);
        of_list_instruction_append(insts, gt);
        of_instruction_goto_table_delete(gt);
    }
}

of_object_t *pack_packet_in(struct netflow *f, size_t *len)
{
    uint32_t pkt_len;
    // uint8_t *buf = NULL;
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
    if (of_packet_in_match_set(of_packet_in, &match) != OF_ERROR_NONE) {
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
    // of_object_wire_buffer_steal((of_object_t*) of_packet_in, &buf);
    // buf = (uint8_t*) WBUF_BUF(OF_OBJECT_TO_WBUF(of_packet_in));
    free(of_octets.data);
    // of_packet_in_delete(of_packet_in);
    return of_packet_in;
}

of_object_t *pack_flow_stats_reply(struct flow **flows, uint32_t xid,
                          size_t flow_count)
{
    // uint8_t *buf = NULL;
    of_match_t match;
    of_flow_stats_reply_t *reply = NULL;
    of_list_instruction_t *insts = NULL;
    of_list_flow_stats_entry_t *stats_list = of_list_flow_stats_entry_new(OF_VERSION_1_3);
    of_flow_stats_entry_t *stat_entry = of_flow_stats_entry_new(OF_VERSION_1_3);
    reply = of_flow_stats_reply_new(OF_VERSION_1_3);
    of_flow_stats_reply_xid_set(reply, xid);

    size_t i;
    for (i = 0; i < flow_count; i++) {
        of_flow_stats_entry_table_id_set(stat_entry, flows[i]->table_id);
        /* TODO: Add support to flow duration */
        of_flow_stats_entry_duration_sec_set(stat_entry, 0);
        of_flow_stats_entry_duration_nsec_set(stat_entry, 0);
        of_flow_stats_entry_priority_set(stat_entry, flows[i]->priority);
        of_flow_stats_entry_idle_timeout_set(stat_entry, flows[i]->idle_timeout);
        of_flow_stats_entry_hard_timeout_set(stat_entry, flows[i]->hard_timeout);
        of_flow_stats_entry_cookie_set(stat_entry, flows[i]->cookie);
        of_flow_stats_entry_packet_count_set(stat_entry, flows[i]->pkt_cnt);
        of_flow_stats_entry_byte_count_set(stat_entry, flows[i]->byte_cnt);
        memset(&match, 0, sizeof(match));
        pack_match_fields(&flows[i]->key, &match.fields);
        pack_match_fields(&flows[i]->mask, &match.masks);
        if ( of_flow_stats_entry_match_set(stat_entry, &match) < 0) {
            fprintf(stderr, "%s\n", "Failed to set match of stats flow entry");
        }

        insts = of_list_instruction_new(OF_VERSION_1_3);
        pack_instructions(&flows[i]->insts, insts);
        if ( of_flow_stats_entry_instructions_set(stat_entry, insts) < 0) {
            fprintf(stderr, "%s\n", "Failed to set instructions of stats flow entry");
        }
        of_list_flow_stats_entry_append(stats_list, stat_entry);
        of_list_instruction_delete(insts);
    }
    if (of_flow_stats_reply_entries_set(reply, stats_list) < 0) {
        fprintf(stderr, "%s\n", "Failure to add list of flows to stats reply" );
        return NULL;
    }

    // of_object_wire_buffer_steal((of_object_t*) reply, &buf);
    of_flow_stats_entry_delete(stat_entry);
    of_list_flow_stats_entry_delete(stats_list);
    // of_flow_stats_reply_delete(reply);
    return reply;
}

of_object_t *pack_aggregate_stats_reply(struct flow **flows, 
                                        uint32_t xid, size_t flow_count)
{
    size_t i;
    uint64_t pkt_cnt = 0;
    uint64_t byte_cnt = 0;
    of_flow_stats_reply_t *reply = of_aggregate_stats_reply_new(OF_VERSION_1_3);
    of_aggregate_stats_reply_xid_set(reply, xid);
    of_aggregate_stats_reply_flow_count_set(reply, flow_count);
    
    for (i = 0; i < flow_count; i++) {
        pkt_cnt += flows[i]->pkt_cnt;
        byte_cnt += flows[i]->byte_cnt;
    }
    
    of_aggregate_stats_reply_packet_count_set(reply, pkt_cnt);
    of_aggregate_stats_reply_packet_count_set(reply, byte_cnt);

    return reply;
}