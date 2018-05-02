#include "bgp.h"
#include "lib/util.h"
#include <netemu/netns.h>

static void bgp_start(struct routing *rt, char *rname);
static void bgp_advertise(struct routing *rt);
static char* find_router_id(struct bgp* p);

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
    char intf[MAX_NODE_NAME+10], intf2[MAX_NODE_NAME+10];
    struct bgp *p = (struct bgp*) rt;
    sprintf(intf2, "%s-bgp", rname);
    sprintf(intf,"%s-ext", intf2);
    char *router_id = find_router_id(p);
    setup_veth(rname, intf, intf2, "br0");

    printf("Router id %s\n", router_id);
    if (router_id != NULL) {


        /* Start exabgp */
        netns_run(rname, "env exabgp.daemon.daemonize=true "
              "exabgp.tcp.bind=%s " 
              "exabgp.log.destination=syslog exabgp %s",
              router_id, p->config_file); 
    }
    free(router_id);
}

static void 
bgp_advertise(struct routing *rt)
{
    struct bgp *p = (struct bgp*) rt;
    UNUSED(p);   
}

static char* 
find_router_id(struct bgp* p)
{
    FILE *cfile;
    char *tmp, *ip;
    char line[100];
    cfile = fopen(p->config_file, "r");
    ip = NULL;
    while( fgets(line, sizeof(line), cfile) != NULL )  {
        tmp =  strstr (line,"router-id");
        if (tmp != NULL ){
            ip = xmalloc(20);
            tmp += sizeof("router_id");
            int i = 0;
            while(*tmp != ';'){
                ip[i] = *tmp;
                i++;
                tmp ++;
            }
            ip[i] = '\0';
        }   
    }
    fclose(cfile);
    return ip;
}