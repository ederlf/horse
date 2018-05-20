#include "bgp.h"
#include "lib/util.h"
#include "lib/net_utils.h"
#include <netemu/netns.h>

static void bgp_start(struct routing *rt, char *rname);
static void bgp_advertise(struct routing *rt);
static void bgp_clean(struct routing *rt, char *rname);
static void set_router_id(struct bgp* p);

void 
bgp_init(struct bgp *p, char *config_file)
{
    routing_init(&p->base, BGP);
    p->base.start = bgp_start;
    p->base.advertise = bgp_advertise;
    p->base.clean = bgp_clean;
    if (strlen(config_file) < MAX_FILE_NAME_SIZE) {
        memcpy(p->config_file, config_file, MAX_FILE_NAME_SIZE);
        set_router_id(p);
    }
}   

static void 
bgp_start(struct routing *rt, char * rname)
{
    char intf[42], intf2[42];
    char str_ip[INET_ADDRSTRLEN];
    struct in_addr net_ip = {htonl(rt->router_id)};
    struct bgp *p = (struct bgp*) rt;
    sprintf(intf2, "%s-bgp", rname);
    sprintf(intf,"%s-ext", intf2);
    setup_veth(rname, intf, intf2, "br0");
    /* Set ip. Consider only ipv4 now and a single id and mask /16 */
    get_ip_str(net_ip, str_ip, AF_INET);
    set_intf_ip(rname, intf2, str_ip, "16");
    /* Start exabgp */
    netns_run(rname, "env exabgp.daemon.daemonize=true "
          "exabgp.tcp.bind=%s "
          "exabgp.log.destination=syslog exabgp %s",
          str_ip, p->config_file); 
}

static void 
bgp_advertise(struct routing *rt)
{
    struct bgp *p = (struct bgp*) rt;
    UNUSED(p);   
}

static void
bgp_clean(struct routing *rt, char *rname)
{
    UNUSED(rt);
    char intf[42];
    sprintf(intf,"%s-bgp-ext", rname);
    netns_run(NULL, "pkill exabgp");
    delete_intf(intf);
}

/* It consider the exabgp configuration file format only */
static void
set_router_id(struct bgp* p)
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
    if (ip != NULL) {
        get_ip_net(ip, &p->base.router_id, AF_INET);
        p->base.router_id = ntohl(p->base.router_id);
        free(ip);
    }
}