#include "of_unpack.h"
#include "openflow.h"

void 
unpack_match_fields(of_match_fields_t *fields, struct ofl_flow_key *key)
{
    key->in_port = fields->in_port;
    key->in_phy_port = fields->in_phy_port;
    key->metadata = fields-> metadata;
    key->tunnel_id = fields->tunnel_id; 
    memcpy(key->eth_dst, fields->eth_dst.addr, ETH_LEN);
    memcpy(key->eth_src, fields->eth_src.addr, ETH_LEN);
    key->eth_type = fields-> eth_type;
    key->vlan_id = fields->vlan_vid;
    key->vlan_pcp = fields-> vlan_pcp;
    key->ip_dscp = fields->ip_dscp;
    key->ip_ecn = fields->ip_ecn;
    key->ip_proto = fields->ip_proto;
    key->ipv4_src = fields->ipv4_src;
    key->ipv4_dst = fields->ipv4_dst;
    key->tcp_src = fields->tcp_src;
    key->tcp_dst = fields->tcp_dst;
    key->udp_src = fields->udp_src;
    key->udp_dst = fields->udp_dst;
    key->sctp_src = fields->sctp_src;
    key->sctp_dst = fields->sctp_dst;
    key->icmpv4_type = fields->icmpv4_type;
    key->icmpv4_code = fields->icmpv4_code;
    key->arp_op = fields->arp_op;
    key->arp_spa = fields->arp_spa;
    key->arp_tpa = fields->arp_tpa;
    memcpy(key->arp_sha, fields->arp_sha.addr, ETH_LEN);
    memcpy(key->arp_tha, fields->arp_tha.addr, ETH_LEN);
    memcpy(key->ipv6_src, fields->ipv6_src.addr, IPV6_LEN);
    memcpy(key->ipv6_dst, fields->ipv6_dst.addr, IPV6_LEN);
    key->ipv6_flabel = fields->ipv6_flabel;
    key->icmpv6_type = fields->icmpv6_type;
    key->icmpv6_code = fields->icmpv6_code;
    memcpy(key->ipv6_nd_target, fields->ipv6_nd_target.addr, IPV6_LEN);
    memcpy(key->ipv6_nd_sll, fields->ipv6_nd_sll.addr, ETH_LEN);
    memcpy(key->ipv6_nd_tll, fields->ipv6_nd_tll.addr, ETH_LEN);
    key->mpls_label = fields->mpls_label;
    key->mpls_tc = fields->mpls_tc;
    key->mpls_bos = fields->mpls_bos;    
}

static void 
parse_set_field(of_oxm_t *oxm_field, uint8_t type, struct action *act) 
{
    switch (type) {
        case OFPXMT_OFB_VLAN_VID: {
            uint16_t vlan_vid;
            of_oxm_vlan_vid_value_get(oxm_field, &vlan_vid);
            action_set_field_u16(act, SET_VLAN_ID, vlan_vid);
            break;
        }
        case OFPXMT_OFB_VLAN_PCP : {
            uint8_t vlan_pcp;
            of_oxm_vlan_pcp_value_get(oxm_field, &vlan_pcp);
            action_set_field_u8(act, SET_VLAN_PCP, vlan_pcp);
            break;
        }
        case OFPXMT_OFB_ETH_DST: {
            of_mac_addr_t eth_dst;
            of_oxm_eth_dst_value_get(oxm_field, &eth_dst);
            action_set_field_eth_addr(act, SET_ETH_DST, eth_dst.addr);
            break;
        }
        case OFPXMT_OFB_ETH_SRC: {
            of_mac_addr_t eth_src;
            of_oxm_eth_src_value_get(oxm_field, &eth_src);
            action_set_field_eth_addr(act, SET_ETH_SRC, eth_src.addr);
            break;
        }
        case OFPXMT_OFB_ETH_TYPE: {
            uint16_t eth_type;
            of_oxm_eth_type_value_get(oxm_field, &eth_type);
            action_set_field_u16(act, SET_VLAN_ID, eth_type);
            break;
        }
        case OFPXMT_OFB_IP_DSCP: {
            uint8_t ip_dscp;
            of_oxm_ip_dscp_value_get(oxm_field, &ip_dscp);
            action_set_field_u8(act, SET_IP_DSCP, ip_dscp);
            break;
        }
        case OFPXMT_OFB_IP_ECN : {
            uint8_t ip_ecn;
            of_oxm_ip_ecn_value_get(oxm_field, &ip_ecn);
            action_set_field_u8(act, SET_IP_ECN, ip_ecn);
            break;
        }
        case OFPXMT_OFB_IP_PROTO : {
            uint8_t ip_proto;
            of_oxm_ip_proto_value_get(oxm_field, &ip_proto);
            action_set_field_u8(act, SET_IP_PROTO, ip_proto);
            break;
        }
        case OFPXMT_OFB_IPV4_SRC : {
            uint32_t ipv4_src;
            of_oxm_ipv4_src_value_get(oxm_field, &ipv4_src);
            action_set_field_u32(act, SET_IPV4_SRC, ipv4_src);
            break; 
        }
        case OFPXMT_OFB_IPV4_DST : {
            uint32_t ipv4_dst;
            of_oxm_ipv4_dst_value_get(oxm_field, &ipv4_dst);
            action_set_field_u32(act, SET_IPV4_DST, ipv4_dst);
            break; 
        }
        case OFPXMT_OFB_TCP_SRC : {
            uint16_t tcp_src;
            of_oxm_tcp_src_value_get(oxm_field, &tcp_src);
            action_set_field_u16(act, SET_TCP_SRC, tcp_src);
            break;
        }
        case OFPXMT_OFB_TCP_DST : {
            uint16_t tcp_dst;
            of_oxm_tcp_dst_value_get(oxm_field, &tcp_dst);
            action_set_field_u16(act, SET_TCP_DST, tcp_dst);
            break;
        }
        case OFPXMT_OFB_UDP_SRC : {
            uint16_t udp_src;
            of_oxm_udp_src_value_get(oxm_field, &udp_src);
            action_set_field_u16(act, SET_UDP_SRC, udp_src);
            break;
        }
        case OFPXMT_OFB_UDP_DST : {
            uint16_t udp_dst;
            of_oxm_udp_dst_value_get(oxm_field, &udp_dst);
            action_set_field_u16(act, SET_UDP_DST, udp_dst);
            break;
        }
        case OFPXMT_OFB_SCTP_SRC : {
            uint16_t sctp_src;
            of_oxm_sctp_src_value_get(oxm_field, &sctp_src);
            action_set_field_u16(act, SET_SCTP_DST, sctp_src); 
            break;
        }
        case OFPXMT_OFB_SCTP_DST : {
            uint16_t sctp_dst;
            of_oxm_sctp_dst_value_get(oxm_field, &sctp_dst);
            action_set_field_u16(act, SET_SCTP_DST, sctp_dst);
            break;
        }
        case OFPXMT_OFB_ICMPV4_TYPE : {
            uint8_t icmpv4_type;
            of_oxm_icmpv4_type_value_get(oxm_field, &icmpv4_type);
            action_set_field_u8(act, SET_ICMPV4_TYPE, icmpv4_type);
            break;
        }
        case OFPXMT_OFB_ICMPV4_CODE : {
            uint8_t icmpv4_code;
            of_oxm_icmpv4_code_value_get(oxm_field, &icmpv4_code);
            action_set_field_u8(act, SET_ICMPV4_CODE, icmpv4_code);
            break;
        }
        case OFPXMT_OFB_ARP_OP : {
            uint16_t arp_op;
            of_oxm_arp_op_value_get(oxm_field, &arp_op);
            action_set_field_u16(act, SET_ARP_OP, arp_op); 
        }
        case OFPXMT_OFB_ARP_SPA : {
            uint32_t arp_spa;
            of_oxm_arp_spa_value_get(oxm_field, &arp_spa);
            action_set_field_u32(act, SET_ARP_SPA, arp_spa);
            break;   
        }
        case OFPXMT_OFB_ARP_TPA : {
            uint32_t arp_tpa;
            of_oxm_arp_tpa_value_get(oxm_field, &arp_tpa);
            action_set_field_u32(act, SET_ARP_TPA, arp_tpa);
            break; 
        }
        case OFPXMT_OFB_ARP_SHA : {
            of_mac_addr_t arp_sha;
            of_oxm_arp_sha_value_get(oxm_field, &arp_sha);
            action_set_field_eth_addr(act, SET_ARP_SHA, arp_sha.addr);
            break;
        }
        case OFPXMT_OFB_ARP_THA : {
            of_mac_addr_t arp_tha;
            of_oxm_arp_tha_value_get(oxm_field, &arp_tha);
            action_set_field_eth_addr(act, SET_ARP_THA, arp_tha.addr);
            break;
        }
        case OFPXMT_OFB_IPV6_SRC : {
            of_ipv6_t ipv6_src;
            of_oxm_ipv6_src_value_get(oxm_field, &ipv6_src);
            action_set_field_ipv6_addr(act, SET_IPV6_SRC, ipv6_src.addr);
            break;
        }
        case OFPXMT_OFB_IPV6_DST : {
            of_ipv6_t ipv6_dst;
            of_oxm_ipv6_src_value_get(oxm_field, &ipv6_dst);
            action_set_field_ipv6_addr(act, SET_IPV6_DST, ipv6_dst.addr);
            break;
        }
        case OFPXMT_OFB_IPV6_FLABEL : {
            uint32_t ipv6_flabel;
            of_oxm_ipv6_flabel_value_get(oxm_field, &ipv6_flabel);
            action_set_field_u32(act, SET_IPV6_FLABEL, ipv6_flabel);
            break;
        }
        case OFPXMT_OFB_ICMPV6_TYPE : {
            uint8_t icmpv6_type;
            of_oxm_icmpv6_type_value_get(oxm_field, &icmpv6_type);
            action_set_field_u8(act, SET_ICMPV6_TYPE, icmpv6_type);
            break;
        }
        case OFPXMT_OFB_ICMPV6_CODE : {
            uint8_t icmpv6_code;
            of_oxm_icmpv6_code_value_get(oxm_field, &icmpv6_code);
            action_set_field_u8(act, SET_ICMPV6_CODE, icmpv6_code);
            break;
        }
        case OFPXMT_OFB_IPV6_ND_TARGET : {
            of_ipv6_t ipv6_nd_target;
            of_oxm_ipv6_nd_target_value_get(oxm_field, &ipv6_nd_target);
            action_set_field_ipv6_addr(act, SET_IPV6_ND_TARGET, 
                                       ipv6_nd_target.addr);
            break;
        }
        case OFPXMT_OFB_IPV6_ND_SLL : {
            of_mac_addr_t ipv6_nd_sll;
            of_oxm_ipv6_nd_sll_value_get(oxm_field, &ipv6_nd_sll);
            action_set_field_eth_addr(act, SET_IPV6_ND_SLL, 
                                      ipv6_nd_sll.addr);
            break;
        }
        case OFPXMT_OFB_IPV6_ND_TLL : {
            of_mac_addr_t ipv6_nd_tll;
            of_oxm_ipv6_nd_tll_value_get(oxm_field, &ipv6_nd_tll);
            action_set_field_eth_addr(act, SET_IPV6_ND_TLL, 
                                      ipv6_nd_tll.addr);
            break;
        }
        case OFPXMT_OFB_MPLS_LABEL : {
            uint32_t mpls_label;
            of_oxm_mpls_label_value_get(oxm_field, &mpls_label);
            action_set_field_u32(act, SET_MPLS_LABEL, mpls_label);
            break;
        }
        case OFPXMT_OFB_MPLS_TC : {
            uint8_t mpls_tc;
            of_oxm_mpls_tc_value_get(oxm_field, &mpls_tc);
            action_set_field_u8(act, SET_MPLS_TC, mpls_tc);
            break;
        }
        case OFPXMT_OFB_MPLS_BOS : {
            uint8_t mpls_bos;
            of_oxm_mpls_bos_value_get(oxm_field, &mpls_bos);
            action_set_field_u8(act, SET_MPLS_BOS, mpls_bos);
            break;
        }
        case OFPXMT_OFB_TUNNEL_ID : {
            uint64_t tunnel_id;
            of_oxm_tunnel_id_value_get(oxm_field, &tunnel_id);
            action_set_field_u8(act, SET_TUNNEL_ID, tunnel_id);
            break;
        }
        default : {
            break;
        }        
    }
}

#define action_add(type, dst, gen_act) action_##type##_add(dst, gen_act)

#define unpack_actions(type, wire_actions, actions) \
do { \
    uint16_t rv, act_type; \
    of_action_t act; \
    struct action gen_act; \
    of_oxm_t *oxm_field; \
    OF_LIST_ACTION_ITER(wire_actions, &act, rv) { \
        of_wire_buffer_u16_get (act.wbuf, act.obj_offset, &act_type); \
        switch (act_type) { \
            case OFPAT_OUTPUT: { \
                uint32_t port; \
                of_action_output_port_get(&act, &port); \
                action_output(&gen_act, port); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_SET_FIELD: { \
                uint8_t field_type; \
                oxm_field = of_action_set_field_field_get (&act); \
                of_wire_buffer_u8_get (oxm_field->wbuf, oxm_field->obj_offset + 2, &field_type); \
                parse_set_field(oxm_field, field_type, &gen_act); \
                break; \
            } \
            case OFPAT_GROUP: { \
                uint32_t group_id; \
                of_action_group_group_id_get (&act, &group_id); \
                action_group(&gen_act, group_id); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_PUSH_VLAN: { \
                uint16_t eth_type; \
                of_action_push_vlan_ethertype_get(&act, &eth_type); \
                action_push_vlan(&gen_act, eth_type); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_PUSH_MPLS: { \
                uint16_t eth_type; \
                of_action_push_mpls_ethertype_get(&act, &eth_type); \
                action_push_mpls(&gen_act, eth_type); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_POP_VLAN: { \
                action_pop_vlan(&gen_act); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_POP_MPLS: { \
                uint16_t eth_type; \
                of_action_pop_mpls_ethertype_get(&act, &eth_type); \
                action_pop_mpls(&gen_act, eth_type); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_COPY_TTL_OUT: { \
                action_copy_ttl_out(&gen_act); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_COPY_TTL_IN: { \
                action_copy_ttl_in(&gen_act); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_DEC_MPLS_TTL: { \
                action_dec_mpls_ttl(&gen_act); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            case OFPAT_SET_MPLS_TTL: { \
                uint8_t mpls_ttl; \
                of_action_set_mpls_ttl_mpls_ttl_get (&act, &mpls_ttl); \
                action_mpls_ttl(&gen_act, mpls_ttl); \
                action_add(type, &actions, gen_act); \
                break; \
            } \
            default: { \
                break; \
            } \
        } \
    } \
} while(0) \

void 
unpack_instructions(of_list_instruction_t *insts, struct instruction_set *is)
{
    uint16_t inst_type;
    int rv; 
    of_list_action_t *action_list;
    of_instruction_t inst; //= of_object_new (OF_VERSION_1_3);
    instruction_set_init(is);
     OF_LIST_INSTRUCTION_ITER (insts, &inst, rv) {
        of_wire_buffer_u16_get (inst.wbuf, 0, &inst_type);

        switch (inst_type) {
            case OFPIT_GOTO_TABLE: {
                uint8_t table_id;
                struct goto_table gt;
                of_instruction_goto_table_table_id_get(&inst, &table_id);
                inst_goto_table(&gt, table_id);               
                add_goto_table(is, gt);
                break;
            }
            case OFPIT_WRITE_METADATA: {
                uint64_t metadata;
                struct write_metadata wm;
                of_instruction_write_metadata_metadata_get(&inst, &metadata);
                inst_write_metadata(&wm, metadata); 
                add_write_metadata(is, wm);
                break;
            }
            case OFPIT_WRITE_ACTIONS: {
                struct write_actions wa;
                struct action_set as;
                action_set_init(&as);
                action_list = of_instruction_apply_actions_actions_get(&inst);
                unpack_actions(set, action_list, as);
                inst_write_actions(&wa, as);
                add_write_actions(is, wa);
                of_object_delete(action_list);
                break;
            }
            case OFPIT_APPLY_ACTIONS: {
                struct apply_actions aa;
                struct action_list al;
                action_list_init(&al);
                action_list = of_instruction_apply_actions_actions_get(&inst);
                unpack_actions(list, action_list, al);
                inst_apply_actions(&aa, al);
                add_apply_actions(is, aa);
                of_object_delete(action_list);
                break;
            }
            case OFPIT_CLEAR_ACTIONS: {
                struct clear_actions clear;
                inst_clear_actions(&clear);
                add_clear_actions(is, clear);
                break;
            }
            default:
                break;
        }
    }
}

int
unpack_flow_mod(of_object_t *obj, struct flow *f)
{
    uint8_t table_id;
    uint16_t idle_timeout, hard_timeout, flags;
    uint16_t priority;
    uint64_t cookie;
    of_match_t match;
    of_list_instruction_t *insts = NULL;

    of_flow_modify_t *fm = (of_flow_modify_t*) obj;
    of_flow_modify_table_id_get(fm, &table_id);
    f->table_id = table_id;
    of_flow_modify_idle_timeout_get(fm, &idle_timeout);
    f->idle_timeout = idle_timeout;
    of_flow_modify_hard_timeout_get(fm, &hard_timeout);
    f->hard_timeout = hard_timeout;
    of_flow_modify_flags_get(fm, &flags);
    f->flags = flags;
    of_flow_modify_cookie_get(fm, &cookie);
    f->cookie = cookie;
    of_flow_modify_priority_get(fm, &priority);
    f->priority = priority;

    if (of_flow_modify_match_get(obj, &match) < 0) {
        fprintf(stderr, "Failed to get match");
        return -1;
    }
    unpack_match_fields(&match.fields, &f->key);
    unpack_match_fields(&match.masks, &f->mask);

    insts = of_flow_modify_instructions_get(obj); 
    if (insts == NULL) {
        fprintf(stderr, "Failed to get instructions");
        return -1;
    }    
    unpack_instructions(insts, &f->insts);
    of_object_delete(insts);
    return 0;
}

int 
unpack_flow_stats_request(of_object_t *obj)
{
    uint64_t cookie, cookie_mask;
    uint32_t xid, out_port, out_group;
    uint8_t table_id;
    of_match_t match;
    of_flow_stats_request_t *req = (of_flow_stats_request_t*) obj;

    of_flow_stats_request_xid_get(req, &xid);
    of_flow_stats_request_cookie_get(req, &cookie);
    of_flow_stats_request_cookie_mask_get(req, &cookie_mask);
    of_flow_stats_request_table_id_get(req, &table_id);
    of_flow_stats_request_out_port_get(req, &out_port);
    of_flow_stats_request_out_group_get(req, &out_group);

    if ( of_flow_stats_request_match_get(obj, &match) < 0 ) {
        fprintf(stderr, "Failed to get flow stats match");
        return -1;
    }

    // unpack_match_fields(&match.fields, )
    return 0;
}

int 
unpack_packet_out(of_object_t *obj, struct netflow* f, struct action_list *al)
{
    of_octets_t octets[1];
    uint8_t *pkt;
    of_packet_out_t *pkt_out = (of_packet_out_t*) obj;
    of_list_action_t *action_list = of_packet_out_actions_get(pkt_out);
    of_packet_out_buffer_id_get(pkt_out, &f->metadata.buffer_id);
    of_packet_out_in_port_get(pkt_out, &f->match.in_port);
    of_packet_out_data_get(pkt_out, octets);
    unpack_actions(list, action_list, *al);
    pkt = (uint8_t*) octets->data;
    if (pkt != NULL){
        pkt_to_netflow(pkt, f, octets->bytes);
    }
    of_object_delete(action_list);
    return 0;
}

