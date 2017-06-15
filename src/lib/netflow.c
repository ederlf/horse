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
    int *top = &nf->tags.top;
    struct tag *new;
    if ( *top < (MAX_STACK_SIZE - 1) && 
        (eth_type == ETH_TYPE_VLAN || 
         eth_type == ETH_TYPE_VLAN_QinQ) ) {
        bool empty = *top == STACK_EMPTY? true: false;
        (*top)++;
        new = &nf->tags.level[*top];
        new->type = eth_type;

        if (!empty) {
            /* Copies the previous vlan tag to the new in the stack */
            struct tag *prev = &nf->tags.level[*top -1];
            if (prev->type == ETH_TYPE_VLAN || 
                prev->type == ETH_TYPE_VLAN_QinQ ) {
                new->vlan_tag.tag = prev->vlan_tag.tag;
            }
        }
        else {
            new->vlan_tag.tag = 0;
        }
        /* set eth_type of vlan type */
        new->vlan_tag.ethertype = eth_type;
    }
}

void netflow_push_mpls(struct netflow *nf, uint16_t eth_type)
{
    int *top = &nf->tags.top;
    struct tag *new;
    /* eth_type must be 0x8847 or 0x8848 */
    if ( *top < (MAX_STACK_SIZE - 1) && 
        (eth_type == ETH_TYPE_MPLS || 
         eth_type == ETH_TYPE_MPLS_MCAST) ) {
        bool prev_empty = *top == STACK_EMPTY? true: false;
        (*top)++;
        new = &nf->tags.level[*top];
        new->type = eth_type;

        if (!prev_empty) {
            struct tag *prev = &nf->tags.level[*top -1];
            if(prev->type == ETH_TYPE_MPLS || 
                prev->type == ETH_TYPE_MPLS_MCAST) {
                /* Copies the previous mpls to the new in the stack 
                 * No need to copy the match values as they are already 
                 * set to the old ones. 
                 * Set S bit to 0 */
                new->mpls_tag.fields = prev->mpls_tag.fields;
                new->mpls_tag.fields &= ~MPLS_S_MASK;
                nf->match.mpls_bos = 0;
            }
        }
        else {
            /* Set S bit to 1*/
            new->mpls_tag.fields = MPLS_S_MASK;
            nf->match.mpls_bos = 1;
        }
        /* Change eth_type to mpls type */
        nf->match.eth_type = eth_type; 
    }
}

void netflow_pop_vlan(struct netflow *nf)
{
    int *top = &nf->tags.top;
    if (netflow_is_vlan_tagged(nf)) {
        struct tag *t = &nf->tags.level[*top];
        /* Reset type */
        t->type = 0;
        (*top)--;
        if(netflow_is_vlan_tagged(nf)){
            t = &nf->tags.level[*top];
            /* Set the vlan_vid and pcp */
            nf->match.vlan_id = (t->vlan_tag.tag  & VLAN_VID_MASK) >> 
                                 VLAN_VID_SHIFT;
            nf->match.vlan_pcp = (t->vlan_tag.tag  & VLAN_PCP_MASK) >>                  VLAN_PCP_SHIFT;
        }
        else {
            nf->match.vlan_id = nf->match.vlan_pcp = 0;
        }
    } 
}

void netflow_pop_mpls(struct netflow *nf, uint16_t eth_type)
{
    int *top = &nf->tags.top;
    if (*top >= 0) {
        struct tag *t = &nf->tags.level[*top];
        /* Reset type */
        t->type = 0;
        (*top)--;
        nf->match.eth_type = eth_type;
        if(netflow_is_outer_mpls(nf)){
            /* Set the match fields */
            nf->match.mpls_label = (t->mpls_tag.fields & MPLS_LABEL_MASK) >>                    MPLS_LABEL_SHIFT;
            nf->match.mpls_tc = (t->mpls_tag.fields & MPLS_TC_MASK) >>                    MPLS_TC_SHIFT;
            nf->match.mpls_bos = (t->mpls_tag.fields & MPLS_S_MASK) >>                    MPLS_S_SHIFT;
        }
        else {
            nf->match.mpls_label = nf->match.mpls_tc = nf->match.mpls_bos = 0;
        }
    } 
}

/* Checks if the outermost tag is a VLAN */
bool netflow_is_outer_mpls(struct netflow *nf)
{
    return  nf->tags.top != STACK_EMPTY && 
            (nf->tags.level[nf->tags.top].type == ETH_TYPE_MPLS ||
             nf->tags.level[nf->tags.top].type == ETH_TYPE_MPLS);
}


/* Checks if the outermost tag is a VLAN */
bool netflow_is_vlan_tagged(struct netflow *nf)
{
    return  nf->tags.top != STACK_EMPTY && 
            (nf->tags.level[nf->tags.top].type == ETH_TYPE_VLAN ||
             nf->tags.level[nf->tags.top].type == ETH_TYPE_VLAN_QinQ);
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
