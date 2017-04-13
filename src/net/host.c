#include "host.h"
#include "legacy_node.h"
#include "lib/util.h"

struct host {
    struct legacy_node ep; /* Node is an endpoint */
    /* Add stuff possibly stuff related to a node */
};

struct host *
host_new(void)
{
    struct host *h = xmalloc(sizeof(struct host));
    legacy_node_init(&h->ep, HOST);
    return h;
}

void 
host_destroy(struct host *h)
{
    legacy_node_clean(&h->ep);
    free(h);
}