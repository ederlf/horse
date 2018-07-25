#include "exabgp_daemon.h"
#include "lib/util.h"
#include <netemu/netns.h>
#include <uthash/utarray.h>
#include <unistd.h>

static void start_exabgp(struct routing_daemon *);
static void stop_exabgp(struct routing_daemon *);

struct exabgp_daemon {
    struct routing_daemon base;
    char *config_file;
    UT_array *ips;
};

struct exabgp_daemon* 
exabgp_daemon_new(char *namespace)
{
    struct exabgp_daemon *d = xmalloc(sizeof(struct exabgp_daemon));
    d->base.type = EXABGP;
    strncpy(d->base.namespace, namespace, MAX_NAMESPACE_ID);
    d->config_file = NULL;
    utarray_new(d->ips, &ut_str_icd);
    d->base.start = start_exabgp;
    d->base.stop = stop_exabgp;
    return d;
}

static void
start_exabgp(struct routing_daemon* r)
{
    char *buf;
    FILE *stream;
    size_t len;
    struct exabgp_daemon *d = (struct exabgp_daemon*) r;
    char **addr = NULL;
    stream = open_memstream(&buf, &len);
    // char ip[INET_ADDRSTRLEN];
    fprintf(stream, "\" ");
    while ( (addr=(char**)utarray_next(d->ips, addr))) {
        
        // memset(ip, 0x0, INET_ADDRSTRLEN);
        // char *p = strchr(*addr,'/');
        // printf("Buf is %s\n", *addr);
        // strncpy(ip, *addr, p - (*addr));
        fprintf(stream, "%s ", *addr);

    }
    fprintf(stream, "\"");
    fclose(stream);
    /* Start exabgp */
    printf("%s\n", d->config_file);
    printf("%s\n", buf);
    netns_launch(d->base.namespace, "env exabgp.daemon.daemonize=true "
          "exabgp.tcp.bind=%s "
          "exabgp.log.level=NOTICE "
          "exabgp.log.destination=syslog exabgp %s",
          buf, d->config_file);
    free(buf);
}

static void
stop_exabgp(struct routing_daemon *r)
{
    struct exabgp_daemon *d = (struct exabgp_daemon*) r;
    netns_launch(NULL, "pkill -9 exabgp");
    free(d->config_file);
    utarray_free(d->ips);
}

void 
set_exabgp_daemon_config_file(struct exabgp_daemon *d, char *fname)
{
    d->config_file = xmalloc(strlen(fname)+1);
    strcpy(d->config_file, fname);
}

void
exabgp_daemon_add_port_ip(struct exabgp_daemon *d, char *ip)
{
    utarray_push_back(d->ips, &ip);
}

