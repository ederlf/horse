#include "dp_actions.h"
#include "lib/util.h"
#include <uthash/utlist.h>

static void 
pop_vlan(struct action *act, struct netflow *nf)
{
    netflow_pop_vlan(nf);
    UNUSED(act);
}

static void 
push_vlan(struct action *act, struct netflow *nf)
{
    netflow_push_vlan(nf, act->psh.eth_type);
}

static void 
push_mpls(struct action *act, struct netflow *nf)
{
    netflow_push_mpls(nf, act->psh.eth_type);
}

static void 
pop_mpls(struct action *act, struct netflow *nf)
{
    netflow_pop_mpls(nf, act->pop_mpls.eth_type);
}

static void 
set_field(struct action *act, struct netflow *nf)
{
    struct set_field set = act->set;
    switch (set.field) {
        case SET_IN_PORT :{
            nf->match.in_port = set.u32_field;
            break;
        }
        case SET_METADATA: {
            nf->match.metadata = set.u64_field;
            break;
        }
        case SET_TUNNEL_ID: {
            nf->match.tunnel_id = set.u64_field;
            break;
        }
        case SET_ETH_TYPE: {
            nf->match.eth_type =  set.u16_field;
            break;
        }
        case SET_ETH_DST: {
            memcpy(nf->match.eth_dst, set.eth_addr, 6);
            break;
        }
        case SET_ETH_SRC: {
            memcpy(nf->match.eth_src, set.eth_addr, 6);
            break;
        }
        case SET_VLAN_ID: {
            if (netflow_is_vlan_tagged(nf)){
                uint16_t *tag = &nf->tags.level[nf->tags.top].vlan_tag.tag;
                nf->match.vlan_id = set.u16_field;
                *tag = (*tag & ~VLAN_VID_MASK) | (nf->match.vlan_id & VLAN_VID_MASK);
            }
            break;
        }
        case SET_VLAN_PCP: {
            if (netflow_is_vlan_tagged(nf)){
                uint16_t *tag = &nf->tags.level[nf->tags.top].vlan_tag.tag;
                nf->match.vlan_pcp = set.u8_field;
                *tag = (*tag & ~VLAN_PCP_MASK) | (nf->match.vlan_pcp << VLAN_PCP_SHIFT);
            }
            break;
        }
        case SET_MPLS_LABEL: {
            if (netflow_is_outer_mpls(nf)){
                uint32_t *fields = &nf->tags.level[nf->tags.top].mpls_tag.fields;
                nf->match.mpls_label = set.u32_field;
                *fields = (*fields & ~MPLS_LABEL_MASK) | ( (nf->match.mpls_label << MPLS_LABEL_SHIFT) & MPLS_LABEL_MASK);
            }
            break;
        }
        case SET_MPLS_TC: {
            if (netflow_is_outer_mpls(nf)){
                uint32_t *fields = &nf->tags.level[nf->tags.top].mpls_tag.fields;
                nf->match.mpls_tc = set.u8_field;
                *fields = (*fields & MPLS_TC_MASK) | ((nf->match.mpls_tc << MPLS_TC_SHIFT) & MPLS_TC_MASK);
            }
            break;
        }
        case SET_MPLS_BOS: {
            if (netflow_is_outer_mpls(nf)){
                uint32_t *fields = &nf->tags.level[nf->tags.top].mpls_tag.fields;
                nf->match.mpls_bos = set.u8_field;
                *fields = (*fields & MPLS_S_MASK) |
                 ((nf->match.mpls_bos << MPLS_S_SHIFT) & MPLS_S_MASK);
            }
            break;
        }
        case SET_IP_DSCP: {
            nf->match.ip_dscp = set.u8_field;
            break;
        }
        case SET_IP_ECN: {
            nf->match.ip_ecn = set.u8_field;
            break;
        }
        case SET_IP_PROTO: {
            nf->match.ip_proto = set.u8_field;
            break;
        }
        case SET_IPV4_DST: {
            nf->match.ipv4_dst = set.u32_field;
            break;
        }
        case SET_IPV4_SRC: {
            nf->match.ipv4_src = set.u32_field;
            break;
        }
        case SET_TCP_DST: {
            nf->match.tcp_dst = set.u16_field;
            break;
        }
        case SET_TCP_SRC: {
            nf->match.tcp_src = set.u16_field;
        }
        case SET_UDP_DST: {
            nf->match.udp_dst = set.u16_field;
            break;
        }
        case SET_UDP_SRC: {
            nf->match.udp_src = set.u16_field;
            break;
        }
        case SET_ARP_OP: {
            nf->match.arp_op = set.u16_field;
            break;
        }
        case SET_ARP_SPA: {
            nf->match.arp_spa = set.u32_field;
            break;
        }
        case SET_ARP_TPA: {
            nf->match.arp_tpa = set.u32_field;
            break;
        }
        case SET_ARP_SHA: {
            memcpy(nf->match.arp_sha, set.eth_addr, 6);
            break;
        }
        case SET_ARP_THA: {
            memcpy(nf->match.arp_tha, set.eth_addr, 6);
            break;
        }
        case SET_IPV6_DST: {
            memcpy(nf->match.ipv6_dst, set.ipv6_addr, 16);
            break;
        }
        case SET_IPV6_SRC: {
            memcpy(nf->match.ipv6_src, set.ipv6_addr, 16);
            break;
        }
        case SET_IPV6_ND_TARGET: {
            memcpy(nf->match.ipv6_nd_target, set.eth_addr, 6);
            break;
        }
        case SET_IPV6_ND_SLL: {
            memcpy(nf->match.ipv6_nd_sll, set.eth_addr, 6);
            break;
        }
        case SET_IPV6_ND_TLL: {
            memcpy(nf->match.ipv6_nd_tll, set.eth_addr, 6);
            break;
        }
    }
}

static void 
group(struct action *act, struct netflow *nf)
{
    UNUSED(act);
    UNUSED(nf);
}

static void 
output(struct action *act, struct netflow *nf)
{
    struct output out = act->out;
    netflow_add_out_port(nf, out.port);
}

/* Array of function pointers for the actions. */ 
static void (*handle_action[MAX_ACTION_SET]) (struct action *act, struct netflow *nf) = {
    [ACT_POP_VLAN] = pop_vlan, 
    [ACT_POP_MPLS] = pop_mpls,
    [ACT_PUSH_MPLS] = push_vlan,
    [ACT_PUSH_VLAN] = push_mpls,
    [ACT_SET_FIELD] = set_field,
    [ACT_GROUP] = group,
    [ACT_OUTPUT] = output
};

void 
execute_action(struct action *act, struct netflow *flow) {
    /* Execute action based on the type */
    (*handle_action[act->type]) (act, flow);
}