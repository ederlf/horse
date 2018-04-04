#include "bgp.h"
#include "lib/util.h"
#include <netemu/netns.h>



static void bgp_start(struct routing *rt, char *rname);
static void bgp_advertise(struct routing *rt);

void 
bgp_init(struct bgp *p, char *config_file)
{
    routing_init(&p->base, BGP);
    p->base.start = bgp_start;
    p->base.advertise = bgp_advertise;
    if (strlen(config_file) < MAX_FILE_NAME_SIZE) {
        memcpy(p->config_file, config_file, MAX_FILE_NAME_SIZE);
    }
}   

static void 
bgp_start(struct routing *rt, char * rname)
{
    struct bgp *p = (struct bgp*) rt;
    netns_run(rname, "env exabgp.daemon.daemonize=true " 
              "exabgp.log.destination=syslog exabgp %s",
              p->config_file); 
}

static void 
bgp_advertise(struct routing *rt)
{
    struct bgp *p = (struct bgp*) rt;
    UNUSED(p);   
}