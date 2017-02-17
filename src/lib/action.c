#include "action.h"

void 
action_meter(struct action *meter, uint32_t meter_id)
{
    meter->type = ACT_METER;
    meter->meter.meter_id = meter_id;
}

void 
action_copy_ttl_in(struct action *copy_ttl_in)
{
    copy_ttl_in->type = ACT_COPY_TTL_INWARDS;
}

void 
action_pop_vlan(struct action *pop_vlan)
{
    pop_vlan->type = ACT_POP_VLAN;
}

void 
action_pop_mpls(struct action *pop, uint16_t eth_type)
{
    pop->type = ACT_POP_MPLS;
    pop->pop_mpls.eth_type = eth_type;
}

void 
action_push_vlan(struct action *psh, uint16_t eth_type)
{
    psh->type = ACT_PUSH_VLAN;
    psh->psh.eth_type = eth_type;
}

void 
action_push_mpls(struct action *psh, uint16_t eth_type)
{
    psh->type = ACT_POP_MPLS;
    psh->psh.eth_type = eth_type;   
}

void 
action_copy_ttl_out(struct action *copy_ttl_out)
{
    copy_ttl_out->type = ACT_COPY_TTL_OUTWARDS;
}

void 
action_dec_ttl(struct action *dec_ttl)
{
    dec_ttl->type = ACT_DECREMENT_TTL;
}
void 
action_dec_mpls_ttl(struct action *dec_ttl)
{
    dec_ttl->type = ACT_DECREMENT_MPLS_TTL;
}

void 
action_mpls_ttl(struct action *smttl, uint8_t new_ttl)
{
    smttl->type = ACT_SET_MPLS_TTL;
    smttl->set_mpls_ttl.new_ttl = new_ttl;
}

static void 
set_field(struct action *sf, uint8_t field)
{
    sf->type = ACT_SET_FIELD;
    sf->set.field = field;  
}

void action_set_field_u8(struct action *sf, uint8_t field, uint8_t value)
{
    set_field(sf, field);
    sf->set.u8_field = value;
}

void action_set_field_u16(struct action *sf, uint8_t field, uint16_t value)
{
    set_field(sf, field);
    sf->set.u16_field = value;
}

void action_set_field_u32(struct action *sf, uint8_t field, uint32_t value)
{
    set_field(sf, field);
    sf->set.u32_field = value;
}

void action_set_field_u64(struct action *sf, uint8_t field, uint64_t value)
{
    set_field(sf, field);
    sf->set.u64_field = value;
}

void action_set_field_eth_addr(struct action *sf, uint8_t field, uint8_t *eth_addr)
{
    set_field(sf, field);
    memcpy(sf->set.eth_addr, eth_addr, ETH_LEN);
}

void action_set_field_ipv6_addr(struct action *sf, uint8_t field, uint8_t *ipv6_addr)
{
    set_field(sf, field);
    memcpy(sf->set.eth_addr, ipv6_addr, IPV6_LEN); 
}

void
action_output(struct action *output, uint32_t port)
{
    output->type = ACT_OUTPUT;
    output->out.port = port;
}


