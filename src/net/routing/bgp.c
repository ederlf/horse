#include "bgp.h"
#include "routing_msg.h"
#include "lib/util.h"
#include "lib/net_utils.h"
#include <netemu/netns.h>
#include <time.h>

static void bgp_init(struct bgp *p, char *config_file);
static void bgp_start(struct routing *rt, char *rname);
static struct routing_msg* bgp_announce(struct bgp *p, uint32_t neighbor);
static void bgp_clean(struct routing *rt, char *rname);
static void set_router_id(struct bgp* p);

struct adv {
    uint32_t ip;
    uint8_t cidr;
    UT_hash_handle hh; /* Make it hashable, easier to change configuration */
}; 

struct neighbor {
    uint32_t ip;
    uint32_t as;
    uint8_t state;
    uint8_t reachable;
    UT_hash_handle hh; /* Make it hashable */
};

struct bgp {
    struct routing base;
    char config_file[MAX_FILE_NAME_SIZE]; /* Will be removed when config is done in Python */
    uint16_t neighbor_num;
    struct neighbor *neighbors;
    struct adv *prefixes;
};

struct bgp*
bgp_new(char* config_file)
{
    struct bgp *p = xmalloc(sizeof(struct bgp));
    bgp_init(p, config_file);
    return p;
}

static void 
bgp_init(struct bgp *p, char *config_file)
{
    routing_init(&p->base, BGP);
    p->base.start = bgp_start;
    p->base.clean = bgp_clean;
    if (strlen(config_file) < MAX_FILE_NAME_SIZE) {
        memcpy(p->config_file, config_file, MAX_FILE_NAME_SIZE);
        set_router_id(p);
    }
    p->neighbors = NULL;
    p->prefixes = NULL;
}   

static void 
bgp_start(struct routing *rt, char * rname)
{
    char intf[42], intf2[42];
    UNUSED(rt);
    char str_ip[INET_ADDRSTRLEN];
    struct in_addr net_ip = {htonl(rt->router_id)};
    struct bgp *p = (struct bgp*) rt;

    sprintf(intf2, "%s-bgp", rname);
    sprintf(intf,"%s-ext", intf2);
    get_ip_str(net_ip, str_ip, AF_INET);
    setup_veth(rname, intf, intf2, str_ip, "16", "br0");
    /* Start exabgp */
    netns_run(rname, "env exabgp.daemon.daemonize=true "
          "exabgp.tcp.bind=%s "
          "exabgp.log.destination=syslog exabgp %s",
          str_ip, p->config_file); 
}

void 
bgp_add_neighbor(struct bgp *p, uint32_t neighbor_ip, uint32_t neighbor_as)
{   
    struct neighbor *neghb = xmalloc(sizeof(struct neighbor));
    neghb->ip = neighbor_ip;
    neghb->as = neighbor_as;
    neghb->state = BGP_STATE_DOWN;
    neghb->state = 0;
    p->neighbor_num++;
    HASH_ADD(hh, p->neighbors, ip, sizeof(uint32_t), neghb);
}

void 
bgp_add_adv_prefix(struct bgp *p, uint32_t prefix, uint8_t cidr)
{
    struct adv *adv= xmalloc(sizeof(struct adv));
    adv->ip = prefix;
    adv->cidr = cidr;
    HASH_ADD(hh, p->prefixes, ip, sizeof(struct adv), adv);
}

struct routing_msg*
bgp_handle_state_msg(struct bgp *p, struct bgp_state *msg)
{
    uint32_t neighbor = msg->peer_rid;
    if (msg->state == BGP_STATE_UP) {
        return bgp_announce(p, neighbor);
    }
    // else if(msg->state  == BGP_STATE_DOWN) {

    // }
    return NULL;
}

/*  TODO: In a first moment it announces all prefixes to every neighbor. 
    It is enough for the IXP case.
    Filtering options will be implemented later on with every prefix. 
*/ 
static struct routing_msg*
bgp_announce(struct bgp *p, uint32_t neighbor)
{
    struct neighbor *n;
    struct adv *cur_prefix, *ptmp;
    HASH_FIND(hh, p->neighbors, &neighbor, sizeof(uint32_t), n);
    if (n != NULL) {
        HASH_ITER(hh, p->prefixes, cur_prefix, ptmp) {
               
        } 
    }
    return NULL;
}

static void
bgp_clean(struct routing *rt, char *rname)
{
    struct bgp *p = (struct bgp*) rt;
    struct neighbor *cur_n, *ntmp;
    struct adv *cur_prefix, *ptmp;
    char intf[42];
    sprintf(intf,"%s-bgp-ext", rname);
    netns_launch(NULL, "pkill exabgp");
    HASH_ITER(hh, p->neighbors, cur_n, ntmp) {
        HASH_DEL(p->neighbors, cur_n);  
        free(cur_n);
    }
    HASH_ITER(hh, p->prefixes, cur_prefix, ptmp) {
        HASH_DEL(p->prefixes, cur_prefix);  
        free(cur_prefix);
    }
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
            while(*tmp != ' ' && *tmp != ';'){
                ip[i] = *tmp;
                i++;
                tmp++;
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

uint32_t 
bgp_router_id(struct bgp* p)
{
    return p->base.router_id;
}