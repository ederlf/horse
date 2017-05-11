#include "netflow.h"
#include <uthash/utlist.h>

void netflow_init(struct netflow *nf)
{
    memset(nf, 0x0, sizeof(struct netflow));
    nf->next = NULL;
}

void 
netflow_push_vlan(struct netflow *nf, uint16_t eth_type)
{
    /* eth_type must be 0x8100 or 0x88a8 */
    if ( nf->vlan_stk.top < (MAX_STACK_SIZE - 1) && 
        (eth_type == ETH_TYPE_VLAN || eth_type == ETH_TYPE_VLAN_QinQ) ) {
        bool empty = nf->vlan_stk.top == STACK_EMPTY? true: false;
        nf->vlan_stk.top++;
        if (!empty) {
            /* Copies the previous vlan tag to the new in the stack */
            nf->vlan_stk.level[nf->vlan_stk.top].tag = nf->vlan_stk.level[nf->vlan_stk.top - 1].tag;
        }
        else {
            nf->vlan_stk.level[nf->vlan_stk.top].tag = 0;
        }
        /* set eth_type of vlan type */
        nf->vlan_stk.level[nf->vlan_stk.top].ethertype = eth_type;
    }
}

void netflow_push_mpls(struct netflow *nf, uint16_t eth_type)
{
    /* eth_type must be 0x8847 or 0x8848 */
    if ( nf->mpls_stk.top < (MAX_STACK_SIZE - 1) && 
        (eth_type == ETH_TYPE_MPLS || eth_type == ETH_TYPE_MPLS_MCAST) ) {
        bool empty = nf->mpls_stk.top == STACK_EMPTY? true: false;
        nf->mpls_stk.top++;
        if (!empty) {
            /* Copies the previous mpls to the new in the stack 
             * No need to copy the match values as they are already 
             * set to the old ones. */
            nf->mpls_stk.level[nf->mpls_stk.top].fields = nf->mpls_stk.level[nf->mpls_stk.top-1].fields;
            /* Set S bit to 0*/
            nf->mpls_stk.level[nf->mpls_stk.top].fields &= ~MPLS_S_MASK;
            nf->match.mpls_bos = 0;
            /* TC and label keep the same values */
        }
        else {
            /* Set S bit to 1*/
            nf->mpls_stk.level[nf->mpls_stk.top].fields = MPLS_S_MASK;
            nf->match.mpls_bos = 1;
        }
        /* Change eth_type to mpls type */
        nf->match.eth_type = eth_type; 
    }
}

void netflow_pop_vlan(struct netflow *nf)
{
    if (nf->vlan_stk.top >= 0) {
        /* Reset values */
        nf->vlan_stk.level[nf->vlan_stk.top].tag = 0;
        nf->vlan_stk.level[nf->vlan_stk.top].ethertype = 0;
        nf->vlan_stk.top--;
        if(nf->vlan_stk.top != STACK_EMPTY){
            /* Set the vlan_vid and pcp */
            nf->match.vlan_id = (nf->vlan_stk.level[nf->vlan_stk.top].tag  & VLAN_VID_MASK) >> VLAN_VID_SHIFT;
            nf->match.vlan_pcp = (nf->vlan_stk.level[nf->vlan_stk.top].tag  & VLAN_PCP_MASK) >> VLAN_PCP_SHIFT;
        }
        else {
            nf->match.vlan_id = nf->match.vlan_pcp = 0;
        }
    } 
}

void netflow_pop_mpls(struct netflow *nf, uint16_t eth_type)
{
    if (nf->mpls_stk.top >= 0) {
        /* Reset values */
        nf->mpls_stk.level[nf->mpls_stk.top].fields = 0;
        nf->mpls_stk.top--;
        /* Set ethertype */
        nf->match.eth_type = eth_type;
        if(nf->mpls_stk.top != STACK_EMPTY){
            /* Set the match fields */
            nf->match.mpls_label = (nf->mpls_stk.level[nf->mpls_stk.top].fields & MPLS_LABEL_MASK) >> MPLS_LABEL_SHIFT;
            nf->match.mpls_tc = (nf->mpls_stk.level[nf->mpls_stk.top].fields & MPLS_TC_MASK) >> MPLS_TC_SHIFT;
            nf->match.mpls_bos = (nf->mpls_stk.level[nf->mpls_stk.top].fields & MPLS_S_MASK) >> MPLS_S_SHIFT;
        }
        else {
            nf->match.mpls_label = nf->match.mpls_tc = nf->match.mpls_bos = 0;
        }
    } 
}

void netflow_clean_out_ports(struct netflow *flow)
{
    struct out_port *p, *tmp;
    LL_FOREACH_SAFE(flow->out_ports, p, tmp) {
      LL_DELETE(flow->out_ports, p);
      free(p); 
    }
    flow->out_ports = NULL;
}

void 
netflow_update_send_time(struct netflow *flow, uint32_t port_speed)
{
    flow->start_time = flow->end_time;
    /* Port speed is converted to bits per microseconds */
    flow->end_time += (flow->byte_cnt * 8) / ((port_speed * 1000)/1000000);
}
