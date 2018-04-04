#include "routing.h"
#include "bgp.h"
#include "lib/util.h"

void 
routing_init(struct routing *r, uint16_t type)
{
    r->type = type;
    r->asn = 0;
}

struct routing* 
routing_factory(uint16_t type, char *config_file)
{
    switch (type) {
        case BGP: {
            struct bgp *p = xmalloc(sizeof(struct bgp));
            bgp_init(p, config_file);
            return (struct routing*) p; 
        }
    }
    /* Protocol does not exist */
    return NULL;
}