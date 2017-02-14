#include "dp_actions.h"
#include "lib/util.h"
#include <uthash/utlist.h>

static void 
pop_vlan(struct action *act, struct netflow *nf, struct out_port *out_ports)
{
    UNUSED(act);
    UNUSED(nf);
    UNUSED(out_ports);
}

static void 
push_vlan(struct action *act, struct netflow *nf, struct out_port *out_ports)
{
    UNUSED(act);
    UNUSED(nf);
    UNUSED(out_ports);
}

static void 
push_mpls(struct action *act, struct netflow *nf, struct out_port *out_ports)
{
    UNUSED(act);
    UNUSED(nf);
    UNUSED(out_ports);
}

static void 
pop_mpls(struct action *act, struct netflow *nf, struct out_port *out_ports)
{
    UNUSED(act);
    UNUSED(nf);
    UNUSED(out_ports);
}

static void 
set_field(struct action *act, struct netflow *nf, struct out_port *out_ports)
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
            uint16_t *tag = &nf->vlan_stk.level[nf->vlan_stk.top].tag;
            nf->match.vlan_id = set.u16_field;
            *tag = (*tag & ~VLAN_VID_MASK) | (nf->match.vlan_id & VLAN_VID_MASK);
            break;
        }
        case SET_VLAN_PCP: {
            uint16_t *tag = &nf->vlan_stk.level[nf->vlan_stk.top].tag;
            nf->match.vlan_pcp = set.u8_field;
            *tag = (*tag & ~VLAN_PCP_MASK) | (nf->match.vlan_pcp << VLAN_PCP_SHIFT);
            break;
        }
        case SET_MPLS_LABEL: {
            uint32_t *fields = &nf->mpls_stk.level[nf->mpls_stk.top].fields;
            nf->match.mpls_label = set.u32_field;
            *fields = (*fields & ~MPLS_LABEL_MASK) | ( (nf->match.mpls_label << MPLS_LABEL_SHIFT) & MPLS_LABEL_MASK);
            break;
        }
        case SET_MPLS_TC: {
            uint32_t *fields = &nf->mpls_stk.level[nf->mpls_stk.top].fields;
            nf->match.mpls_tc = set.u8_field;
            *fields = (*fields & MPLS_TC_MASK) | ((nf->match.mpls_tc << MPLS_TC_SHIFT) & MPLS_TC_MASK);
            break;
        }
        case SET_MPLS_BOS: {
            uint32_t *fields = &nf->mpls_stk.level[nf->mpls_stk.top].fields;
            nf->match.mpls_bos = set.u8_field;
            *fields = (*fields & MPLS_S_MASK)
                | ((nf->match.mpls_bos << MPLS_S_SHIFT) & MPLS_S_MASK);
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
        case SET_TCP_DST: 
        case SET_UDP_DST: {
            nf->match.tp_dst = set.u16_field;
            break;
        }
        case SET_TCP_SRC: 
        case SET_UDP_SRC: {
            nf->match.tp_src = set.u16_field;
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
    UNUSED(out_ports);
}

static void 
group(struct action *act, struct netflow *nf, struct out_port *out_ports)
{
    UNUSED(act);
    UNUSED(nf);
    UNUSED(out_ports);
}

static void 
output(struct action *act, struct netflow *nf, struct out_port *out_ports)
{
    struct output out = act->out;
    struct out_port *op = xmalloc(sizeof(struct out_port));
    op->port = out.port;
    LL_APPEND(out_ports, op);
    UNUSED(nf);
}

// TODO use log2 [log2(act->type)] to calculate position
/* Array of function pointers for the actions. */ 
static void (*handle_action[MAX_ACTION_SET]) (struct action *act, struct netflow *nf, struct out_port *out_ports) = {
    [2] = pop_vlan, 
    [3] = pop_mpls,
    [4] = push_vlan,
    [5] = push_mpls,
    [10] = set_field,
    [12] = group,
    [13] = output
};

void 
execute_action(struct action *act, struct netflow *flow, struct out_port *out_ports) {

    /* Execute action based on the type */
    (*handle_action[act->type]) (act, flow, out_ports);

}