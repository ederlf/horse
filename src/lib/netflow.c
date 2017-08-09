#include "netflow.h"
#include <uthash/utlist.h>
#include <arpa/inet.h>

void netflow_init(struct netflow *nf)
{
    memset(nf, 0x0, sizeof(struct netflow));
    /* Cannot guarantee that the representation 
     *  of a NULL pointer is all bits 0, so initialize
     *  the pointer. 
    */
    nf->next = NULL;
    nf->out_ports = NULL;
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

void 
netflow_push_mpls(struct netflow *nf, uint16_t eth_type)
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
        /* Reset tag */
        memset(t, 0x0, sizeof(struct tag));
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
        /* Reset tag */
        memset(t, 0x0, sizeof(struct tag));
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


static bool tags_empty(struct netflow *nf)
{
    return nf->tags.top == STACK_EMPTY;
}


static void vlan_to_pkt(struct tag *t, uint8_t *buff)
{
    memcpy(buff, &t->vlan_tag, sizeof(struct vlan)); 
}

static void mpls_to_pkt(struct tag *t, uint8_t *buff)
{
    memcpy(buff, &t->mpls_tag, sizeof(struct mpls)); 
}

/* Turns a netflow into a packet header 
 * The buffer is not owned by the function that just fills
 * it with the header contents. 
 * Returns the number of bytes to be sent */
size_t netflow_to_pkt(struct netflow *nf, uint8_t *buffer)
{
    size_t offset = 0;
    uint8_t *pos = buffer;
    struct flow_key m =  nf->match;
    struct eth_header *eth;
    uint8_t icmp_type, icmp_code;

    eth = (struct eth_header*) pos;
    memcpy(eth->eth_src, m.eth_src, ETH_LEN);
    memcpy(eth->eth_dst, m.eth_dst, ETH_LEN);
    eth->eth_type = htons(m.eth_type);
    offset += sizeof(struct eth_header);
    pos += offset;

    if (!tags_empty(nf)){
        int i;
        int tag_num = nf->tags.top + 1;
        for(i = 0; i < tag_num; ++i) {
            struct tag t = nf->tags.level[i]; 
            if (t.type == ETH_TYPE_MPLS || 
                t.type == ETH_TYPE_MPLS_MCAST){
                mpls_to_pkt(&t, pos);
                offset += sizeof(struct mpls);
                pos += offset;
            }
            else if (t.type == ETH_TYPE_VLAN ||
                     t.type == ETH_TYPE_VLAN_QinQ ) {
                vlan_to_pkt(&t, pos);
                offset += sizeof(struct vlan);
                pos += offset;
            }
        }
    }

    if (m.eth_type == ETH_TYPE_ARP) {
        struct arp_eth_header *arp = (struct arp_eth_header*) pos;
        arp->arp_hrd = htons(ARP_HW_TYPE_ETH); /* Ethernet */
        arp->arp_pro = htons(ETH_TYPE_IP);
        arp->arp_hln = ETH_LEN; 
        arp->arp_pln = sizeof(uint32_t); 
        arp->arp_op = htons(m.arp_op); 
        memcpy(arp->arp_sha, m.arp_sha, ETH_LEN);
        memcpy(arp->arp_sha, m.arp_tha, ETH_LEN);
        arp->arp_spa = htonl(m.arp_spa);
        arp->arp_tpa = htonl(m.arp_tpa);
        offset += sizeof(struct arp_eth_header);
        return offset;
    }
    if (m.eth_type == ETH_TYPE_IP){
        struct ip_header *ip = (struct ip_header*) pos;
        memset(ip, 0x0, sizeof(struct ip_header));
        ip->ip_tos = (ip->ip_tos | m.ip_dscp) << 1 | m.ip_ecn;
        ip->ip_proto = m.ip_proto;
        ip->ip_src = m.ipv4_src;
        ip->ip_dst = m.ipv4_dst;
        offset += sizeof(struct ip_header);
        pos += offset;
        goto l4;
    }
    if (m.eth_type == ETH_TYPE_IPV6){
        struct ipv6_header *ipv6 = (struct ipv6_header*) pos;
        memset(ipv6, 0x0, sizeof(struct ipv6_header));
        ipv6->ipv6_next_hd = m.ip_proto;
        memcpy(ipv6->ipv6_dst, m.ipv6_dst, IPV6_LEN);
        memcpy(ipv6->ipv6_src, m.ipv6_src, IPV6_LEN);
        offset += sizeof(struct ipv6_header);
        pos += offset;
        goto l4;
    }

    l4:
    if (m.ip_proto == IP_PROTO_TCP) {
        struct tcp_header *tcp = (struct tcp_header*) pos;
        memset(tcp, 0x0, sizeof(struct tcp_header));
        tcp->tcp_dst = m.tcp_dst;
        tcp->tcp_src = m.tcp_src;
        offset += sizeof(struct tcp_header);
        pos += offset;
        return offset;
    }
    if (m.ip_proto == IP_PROTO_UDP){
        struct udp_header *udp = (struct udp_header*) pos;
        memset(udp, 0x0, sizeof(struct udp_header));
        udp->udp_dst = m.udp_dst;
        udp->udp_src = m.udp_src;
        offset += sizeof(struct udp_header);
        pos += offset;
        return offset;
    }
    if (m.ip_proto == IP_PROTO_ICMPV4) {
        struct icmp_header *icmp = (struct icmp_header*) pos;
        memset(icmp, 0x0, sizeof(struct icmp_header));
        icmp->icmp_code = icmp_type = m.icmpv4_code;
        icmp->icmp_type = icmp_code = m.icmpv4_code;
        offset += sizeof(struct icmp_header);
        pos += offset;
    }
    if (m.ip_proto == IP_PROTO_ICMPV6) {
        struct icmp_header *icmp = (struct icmp_header*) pos;
        memset(icmp, 0x0, sizeof(struct icmp_header));
        icmp->icmp_code = icmp_type = m.icmpv6_code;
        icmp->icmp_type = icmp_code = m.icmpv6_code;
        offset += sizeof(struct icmp_header);
        pos += offset;
    }

    if (icmp_type == ICMPV6_NEIGH_SOL || icmp_type == ICMPV6_NEIGH_ADV) {
        /* TODO */
    }

    return offset;
}

void 
pkt_to_netflow(uint8_t *buffer, struct netflow *nf, size_t pkt_len)
{
    size_t offset = 0;
    struct flow_key *m =  &nf->match;
    struct eth_header *eth;
    // uint8_t icmp_type, icmp_code;

    eth = (struct eth_header*) buffer;
    m->eth_type = ntohs(eth->eth_type);
    memcpy(m->eth_src, eth->eth_src, ETH_LEN);
    memcpy(m->eth_dst, eth->eth_src, ETH_LEN);
    offset += sizeof(struct eth_header);

    /* VLAN */
    if (m->eth_type == ETH_TYPE_VLAN ||
        m->eth_type == ETH_TYPE_VLAN_QinQ) {

        int *top = &nf->tags.top;
        struct vlan *vlan;
        struct tag *new;
        
        if (pkt_len < offset + sizeof(struct vlan)) {
            return;
        }
        
        vlan = (struct vlan *)(buffer + offset);
        m->vlan_id  = (ntohs(vlan->tag) & VLAN_VID_MASK) >> VLAN_VID_SHIFT;
        m->vlan_pcp = (ntohs(vlan->tag) & VLAN_PCP_MASK) >> VLAN_PCP_SHIFT;
        m->eth_type = ntohs(vlan->ethertype);
        
        /* Add a vlan tag */
        (*top)++;
        new = &nf->tags.level[*top];
        new->type = m->eth_type;
        memcpy(&new->vlan_tag, vlan, sizeof(struct vlan));

        offset += sizeof(struct vlan);
    }

    /* skip through rest of VLAN tags */
    while (m->eth_type  == ETH_TYPE_VLAN ||
           m->eth_type  == ETH_TYPE_VLAN_QinQ) {

        int *top = &nf->tags.top;
        struct vlan *vlan;
        struct tag *new;
        
        if (pkt_len < offset + sizeof(struct vlan)) {
            return;
        }
        
        m->eth_type = ntohs(vlan->ethertype);
        /* Add a vlan tag */
        if (*top < MAX_STACK_SIZE) {
            (*top)++;
            new = &nf->tags.level[*top];
            new->type = m->eth_type;
            memcpy(&new->vlan_tag, vlan, sizeof(struct vlan));    
        }
        else {
            return;
        }       
        offset += sizeof(struct vlan);
    }

    /* MPLS */
    if (m->eth_type== ETH_TYPE_MPLS || m->eth_type== ETH_TYPE_MPLS_MCAST) {
            struct mpls *mpls;
            if (pkt_len < offset + sizeof(struct mpls)) {
                return;
            }
            mpls = (struct mpls *)(buffer + offset);
            m->mpls_label = (ntohl(mpls->fields) & 
                          MPLS_LABEL_MASK) >> MPLS_LABEL_SHIFT;
            m->mpls_tc =    (ntohl(mpls->fields) & 
                             MPLS_TC_MASK) >> MPLS_TC_SHIFT;
            m->mpls_bos =  (ntohl(mpls->fields) & 
                            MPLS_S_MASK) >> MPLS_S_SHIFT;
            offset += sizeof(struct mpls);
            /* no processing past MPLS */
            return;
    }

    /* ARP */
    if (m->eth_type == ETH_TYPE_ARP) {
        if (pkt_len < offset + sizeof(struct arp_eth_header)) {
            return;
        }
        struct arp_eth_header *arp;
        arp = (struct arp_eth_header *)(buffer + offset);
        offset += sizeof(struct arp_eth_header);

        if (ntohs(arp->arp_hrd) == 1 &&
            ntohs(arp->arp_pro) == ETH_TYPE_IP &&
            arp->arp_hln == ETH_LEN &&
            arp->arp_pln == 4) {
            uint16_t arp_op = ntohs(arp->arp_op); 
            if ( arp_op <= 0xff) {
                m->arp_op = arp_op;
            }
            if (arp_op == ARP_OP_REQUEST ||
                arp_op == ARP_OP_REPLY) {
                m->arp_spa = htonl(arp->arp_spa);
                m->arp_tpa = htonl(arp->arp_tpa);
                memcpy(m->arp_sha, arp->arp_sha, ETH_LEN);
                memcpy(m->arp_tha, arp->arp_tha, ETH_LEN);
            }
        }
        return;
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
