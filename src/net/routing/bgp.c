#include "bgp.h"
#include "routing_msg.h"
#include "lib/util.h"
#include "lib/net_utils.h"
#include <netemu/netns.h>
#include <time.h>
#include <unistd.h>
#include <uthash/utarray.h>

static void bgp_init(struct bgp *p, char *config_file);
static void bgp_start(struct routing *rt, char *rname);
static void bgp_clean(struct routing *rt, char *rname);

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
    UT_array *local_ips;
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
    }
    p->neighbors = NULL;
    p->prefixes = NULL;
    utarray_new(p->local_ips, &ut_str_icd);
}   

static void 
bgp_start(struct routing *rt, char * rname)
{
    char intf[42], intf2[42];
    struct bgp *p = (struct bgp*) rt;
    char **addr = NULL;
    int i = 1;
    FILE *stream;
    char *buf;
    size_t len;
    stream = open_memstream(&buf, &len);
    fprintf(stream, "\"");
    char ip[INET_ADDRSTRLEN];
    while ( (addr=(char**)utarray_next(p->local_ips, addr))) {
        memset(ip, 0x0, INET_ADDRSTRLEN);
        char *p = strchr(*addr,'/');
        strncpy(ip, *addr, p - (*addr));
        sprintf(intf2, "%s-bgp%d", rname, i);
        sprintf(intf,"%s-bgp-ext%d", rname, i);
        setup_veth(rname, intf, intf2, ip, p+1, "br0");
        ++i;
        fprintf(stream, "%s ", ip);
    }
    fprintf(stream, "\"");
    fclose(stream);
    /* Start exabgp */
    netns_launch(rname, "env exabgp.daemon.daemonize=true "
          "exabgp.tcp.bind=%s "
          "exabgp.log.level=NOTICE "
          "exabgp.log.destination=syslog exabgp %s",
          buf, p->config_file);
    free(buf);
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
    if (msg->state == BGP_STATE_UP) {
        return (struct routing_msg*) 
        routing_msg_bgp_announce_new(p->base.router_id, msg->peer_rid);
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
    size_t len = utarray_len(p->local_ips);
    size_t i;
    for (i = 0; i < len; ++i){
        sprintf(intf,"%s-bgp-ext%lu", rname, i+1);
        delete_intf(intf);
    }
    netns_launch(NULL, "pkill exabgp");
    HASH_ITER(hh, p->neighbors, cur_n, ntmp) {
        HASH_DEL(p->neighbors, cur_n);
        free(cur_n);
    }
    HASH_ITER(hh, p->prefixes, cur_prefix, ptmp) {
        HASH_DEL(p->prefixes, cur_prefix);
        free(cur_prefix);
    };
    utarray_free(p->local_ips);
}

void 
bgp_set_router_id(struct bgp* p, uint32_t router_id)
{
    p->base.router_id = router_id;
}

uint32_t
bgp_router_id(struct bgp* p)
{
    return p->base.router_id;
}

void
bgp_add_local_ip(struct bgp *p, char *ip)
{
    utarray_push_back(p->local_ips, &ip);
}