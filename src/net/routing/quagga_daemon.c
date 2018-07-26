#include "quagga_daemon.h"
#include "lib/util.h"
#include <netemu/netns.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/un.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h> 

static void start_quagga(struct routing_daemon *);
static void stop_quagga(struct routing_daemon *);

struct quagga_config {
    char *zebra_file;
    char *bgpd_file;
    char *ripd_file;
    char *ripngd_file;
    char *ospfd_file;
    char *ospf6d_file;
};

struct quagga_daemon {
    struct routing_daemon base;
    struct quagga_config config;
    int sock_fd;
    pid_t zebra_pid;
    pid_t bgpd_pid;
    pid_t ospfd_pid;
    pid_t ospf6d_pid;
    pid_t ripd_pid;
    pid_t ripngd_pid;
};

struct quagga_daemon*
quagga_daemon_new(char *namespace)
{
    struct quagga_daemon *d = xmalloc(sizeof(struct quagga_daemon));
    memset(d, 0x0, sizeof(struct quagga_daemon));
    d->base.type = QUAGGA;
    strncpy(d->base.namespace, namespace, MAX_NAMESPACE_ID);
    d->config.zebra_file = NULL;
    d->config.bgpd_file = NULL;
    d->config.ripd_file = NULL;
    d->config.ripngd_file = NULL;
    d->config.ospfd_file = NULL;
    d->config.ospf6d_file = NULL;
    d->base.start = start_quagga;
    d->base.stop = stop_quagga;
    return d;
}

static void
start_quagga(struct routing_daemon *r)
{
    struct quagga_daemon *d = (struct quagga_daemon*) r;
    char sock_location[MAX_NAMESPACE_ID+15];
    sprintf(sock_location, "/tmp/zebra%s.api", d->base.namespace);
    if (d->config.zebra_file != NULL) {
        struct sockaddr_un server;
        char zebra_pid_file[MAX_NAMESPACE_ID+15];      
        sprintf(zebra_pid_file, "/tmp/zebra%s.pid", d->base.namespace);
        d->zebra_pid = netns_run(d->base.namespace,
                                    "zebra -d -f %s -z %s -i %s",
                                    d->config.zebra_file, sock_location, 
                                    zebra_pid_file) + 1;
    
        d->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (d->sock_fd < 0){
            perror("opening stream socket when starting zebra");
            goto error;
        }
        memset(&server, 0x0, sizeof(struct sockaddr_un));
        server.sun_family = AF_UNIX;
        strncpy(server.sun_path, sock_location, sizeof(server.sun_path) - 1);
        while (connect(d->sock_fd, (const struct sockaddr *) &server,
                    sizeof(struct sockaddr_un)) < 0) {
        }
    }
    else {
        perror("Configuration for zebra not found. ");
        goto error;
    }

    if (d->config.bgpd_file != NULL) {
        char bgpd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(bgpd_pid_file, "/tmp/bgpd%s.pid", d->base.namespace);
        d->bgpd_pid = netns_run(d->base.namespace,
                                   "bgpd -d -f %s -z %s -i %s",
                                    d->config.bgpd_file, sock_location,  
                                    bgpd_pid_file) + 1;
    }
    if (d->config.ospfd_file != NULL) {
        char ospfd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ospfd_pid_file, "/tmp/ospfd%s.pid", d->base.namespace);
        d->ospfd_pid = netns_run(d->base.namespace, "ospfd -d -f %s -z %s -i %s",
          d->config.ospfd_file, sock_location,  ospfd_pid_file) + 1;
    }
    if (d->config.ospf6d_file != NULL) {
        char ospf6d_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ospf6d_pid_file, "/tmp/ospf6d%s.pid", d->base.namespace);
        d->ospf6d_pid = netns_run(d->base.namespace, "ospf6d -d -f %s -z %s -i %s",
          d->config.ospf6d_file, sock_location,  ospf6d_pid_file) + 1;
    }
    if (d->config.ripd_file!= NULL) {
        char ripd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ripd_pid_file, "/tmp/ripd%s.pid", d->base.namespace);
        d->ripd_pid = netns_run(d->base.namespace, "ripd -d -f %s -z %s -i %s",
          d->config.ripd_file, sock_location,  ripd_pid_file) + 1;
    }
    if (d->config.ripngd_file != NULL) {
        char ripngd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ripngd_pid_file, "/tmp/ripngd%s.pid", d->base.namespace);
        d->ripngd_pid =  netns_run(d->base.namespace, "ripngd -d -f %s -z %s -i %s",
          d->config.ripngd_file, sock_location,  ripngd_pid_file) + 1;
    }

    /* TODO: Return an error code for the function */
    error:
    return;
}

static void 
stop_quagga(struct routing_daemon *r)
{
    struct quagga_daemon *d = (struct quagga_daemon*) r;
    close(d->sock_fd);
    if (d->zebra_pid){
        kill(d->zebra_pid, SIGKILL);    
    }
    if (d->bgpd_pid){
        kill(d->bgpd_pid, SIGKILL);    
    }
    if (d->ospfd_pid){
        kill(d->ospfd_pid, SIGKILL);    
    }
    if (d->ospf6d_pid){
        kill(d->ospf6d_pid, SIGKILL);    
    }
    if (d->ripd_pid){
        kill(d->ripd_pid, SIGKILL);    
    }
    if (d->ripngd_pid){
        kill(d->ripngd_pid, SIGKILL);    
    }
    free(d->config.zebra_file);
    free(d->config.bgpd_file);
    free(d->config.ospfd_file);
    free(d->config.ospf6d_file);
    free(d->config.ripd_file);
    free(d->config.ripngd_file);
}

void 
set_quagga_daemon_zebra_file(struct quagga_daemon *d, char *fname)
{
    d->config.zebra_file = xmalloc(strlen(fname)+1);
    strcpy(d->config.zebra_file, fname);
}

void 
set_quagga_daemon_bgpd_file(struct quagga_daemon *d, char *fname)
{
    
    d->config.bgpd_file = xmalloc(strlen(fname)+1);
    strcpy(d->config.bgpd_file, fname);
    
}

void 
set_quagga_daemon_ospfd_file(struct quagga_daemon *d, char *fname)
{
    d->config.ospfd_file = xmalloc(strlen(fname)+1);
    strcpy(d->config.ospfd_file, fname);
}

void 
set_quagga_daemon_ospf6d_file(struct quagga_daemon *d, char *fname)
{
    d->config.ospf6d_file = xmalloc(strlen(fname)+1);
    strcpy(d->config.ospf6d_file, fname);
}

void 
set_quagga_daemon_ripd_file(struct quagga_daemon *d, char *fname)
{
    d->config.ripd_file = xmalloc(strlen(fname)+1);
    strcpy(d->config.ripd_file, fname);
}

void 
set_quagga_daemon_ripngd_file(struct quagga_daemon *d, char *fname)
{
    d->config.ripngd_file = xmalloc(strlen(fname)+1);
    strcpy(d->config.ripngd_file, fname);
}