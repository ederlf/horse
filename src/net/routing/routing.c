#include "routing.h"
#include "bgp.h"
#include "lib/util.h"

void 
routing_init(struct routing *r, uint16_t type)
{
    r->type = type;
    r->asn = 0;
    r->router_id = 0;
}