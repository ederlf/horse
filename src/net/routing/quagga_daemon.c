#include "quagga_daemon.h"
#include "lib/util.h"
#include "lib/packets.h"
#include <netemu/netns.h>
#include <errno.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/un.h>
#include <sys/wait.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static void send_ripd_cmd(char* namespace, char* cmd);
static void send_ripngd_cmd(char* namespace, char* cmd);
static void send_ospfd_cmd(char* namespace, char* cmd);
static void send_bgpd_cmd(char* namespace, char* cmd);
static void send_ospf6d_cmd(char* namespace, char* cmd);
static void send_isisd_cmd(char* namespace, char* cmd);
static void start_quagga(struct routing_daemon *, char* );
static void stop_quagga(struct routing_daemon *);
static void change_quagga_config(struct routing_daemon*, char *, uint8_t );


struct quagga_config {
    char *zebra_file;
    char *bgpd_file;
    char *ripd_file;
    char *ripngd_file;
    char *ospfd_file;
    char *ospf6d_file;
    char *isis_file;
};

struct quagga_daemon {
    struct routing_daemon base;
    struct quagga_config config;
    int sock_fd;
    uint32_t router_id;
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
    d->config.isis_file = NULL;
    d->base.start = start_quagga;
    d->base.stop = stop_quagga;
    d->base.change_config = change_quagga_config;
    return d;
}

static int
send_quagga_cmd(char *namespace, char* cmd, uint16_t port)
{
    int sockfd, status;
    struct sockaddr_in sock;
    pid_t pid;
    pid = fork();

    if (pid == -1) {
        perror("fork");
        return -1;
    }
    if (pid != 0) {
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status) == EXIT_SUCCESS ? pid : -1;
    }

    if (netns_enter(namespace) != 0){
        exit(-1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("opening stream socket when trying to send quagga command");
        goto error;
    }

    inet_pton(AF_INET, "127.0.0.1", &sock.sin_addr);
    sock.sin_family = AF_INET;
    sock.sin_port = htons( port );
    if (connect(sockfd , (struct sockaddr *)&sock , sizeof(sock)) < 0){
        perror("connect failed. Error");
        goto error;
    }
    int err = send(sockfd , cmd , strlen(cmd) , 0); 
    if(err   < 0){
        fprintf(stderr, "Send failed %s\n", strerror(errno));
        goto error;
    }
    error:
    close(sockfd);
    exit(0);
}

static void 
send_ripd_cmd(char* namespace, char* cmd)
{
    send_quagga_cmd(namespace, cmd, 2602);
}

static void 
send_ripngd_cmd(char* namespace, char* cmd)
{
    send_quagga_cmd(namespace, cmd, 2603);
}

static void 
send_ospfd_cmd(char* namespace, char* cmd)
{
    send_quagga_cmd(namespace, cmd, 2604);
}

static void 
send_bgpd_cmd(char* namespace, char* cmd)
{
    send_quagga_cmd(namespace, cmd, 2605);
}

static void 
send_ospf6d_cmd(char* namespace, char* cmd)
{
    send_quagga_cmd(namespace, cmd, 2606);
}

static void 
send_isisd_cmd(char* namespace, char* cmd)
{
    send_quagga_cmd(namespace, cmd, 2608);
}

static void
start_quagga(struct routing_daemon *r, char * router_id)
{
    uint32_t rid;
    struct quagga_daemon *d = (struct quagga_daemon*) r;
    char sock_location[MAX_NAMESPACE_ID+15];
    get_ip_net(router_id, &rid, AF_INET);
    d->router_id = ntohl(rid);
    sprintf(sock_location, "/tmp/zebra%s.api", d->base.namespace);
    if (d->config.zebra_file != NULL) {
        struct sockaddr_un server;
        char zebra_pid_file[MAX_NAMESPACE_ID+15];      
        sprintf(zebra_pid_file, "/tmp/zebra%s.pid", d->base.namespace);
        netns_run(d->base.namespace,"zebra -d -f %s -z %s -i %s",
                  d->config.zebra_file, sock_location, zebra_pid_file) ;
    
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
    /* The observed pid of the process is the pid returned by netns_run + 1 
     * Any case it would not be true? 
     */ 
    if (d->config.bgpd_file != NULL) {
        char bgpd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(bgpd_pid_file, "/tmp/bgpd%s.pid", d->base.namespace);
        netns_run(d->base.namespace, "bgpd -d -f %s -z %s -i %s",
                                    d->config.bgpd_file, sock_location,  
                                    bgpd_pid_file);
    }
    if (d->config.ospfd_file != NULL) {
        char ospfd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ospfd_pid_file, "/tmp/ospfd%s.pid", d->base.namespace);
        netns_run(d->base.namespace,"ospfd -d -f %s -z %s -i %s",
                                d->config.ospfd_file, sock_location,  
                                ospfd_pid_file);
    }
    if (d->config.ospf6d_file != NULL) {
        char ospf6d_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ospf6d_pid_file, "/tmp/ospf6d%s.pid", d->base.namespace);
        netns_run(d->base.namespace, "ospf6d -d -f %s -z %s -i %s",
                                d->config.ospf6d_file, sock_location,  
                                ospf6d_pid_file);
    }
    if (d->config.ripd_file!= NULL) {
        char ripd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ripd_pid_file, "/tmp/ripd%s.pid", d->base.namespace);
        netns_run(d->base.namespace, "ripd -d -f %s -z %s -i %s",
                                d->config.ripd_file, sock_location,  
                                ripd_pid_file);
    }
    if (d->config.ripngd_file != NULL) {
        char ripngd_pid_file[MAX_NAMESPACE_ID+15];        
        sprintf(ripngd_pid_file, "/tmp/ripngd%s.pid", d->base.namespace);
        netns_run(d->base.namespace, "ripngd -d -f %s -z %s -i %s",
                                    d->config.ripngd_file, sock_location, 
                                    ripngd_pid_file);
    }
    if (d->config.isis_file != NULL) {
        char isis_pid_file[MAX_NAMESPACE_ID+15];
        sprintf(isis_pid_file, "/tmp/isisd%s/pid", d->base.namespace);
        netns_run(d->base.namespace,"isisd -d -f %s -z %s -i %s",
                  d->config.isis_file, sock_location, isis_pid_file);
    }

    netns_run(d->base.namespace, "/home/vagrant/horse/horse_daemon %s",
              router_id);

    // quagga_daemon_send_bgpd_cmd(d, "horse\nenable\nshow ip bgp summary\nquit\nquit\nquit\n");

    /* TODO: Return an error code for the function */
    error:
    return;
}

static void 
stop_quagga(struct routing_daemon *r)
{
    struct quagga_daemon *d = (struct quagga_daemon*) r;
    close(d->sock_fd);
    if (d->config.zebra_file){
        netns_run(NULL, "pkill -9 -F /tmp/zebra%s.pid", d->base.namespace);
    }
    if (d->config.bgpd_file){
        netns_run(NULL, "pkill -9 -F /tmp/bgpd%s.pid", d->base.namespace);
        // kill(d->bgpd_pid, SIGKILL);    
    }
    if (d->config.ospfd_file){
        netns_run(NULL, "pkill -9 -F /tmp/ospfd%s.pid", d->base.namespace);
    }
    if (d->config.ospf6d_file){
        netns_run(NULL, "pkill -9 -F /tmp/ospf6d%s.pid", d->base.namespace); 
    }
    if (d->config.ripd_file){
        netns_run(NULL, "pkill -9 -F /tmp/ripd%s.pid", d->base.namespace);
    }
    if (d->config.ripngd_file){
        netns_run(NULL, "pkill -9 -F /tmp/ripngd%s.pid", d->base.namespace);
    }
    if (d->config.isis_file) {
        netns_run(NULL, "pkill -9 -F /tmp/isisd%s.pid", d->base.namespace);
    }
    netns_run(NULL, "pkill -9 -F /tmp/daemon%u.pid", d->router_id);
    free(d->config.zebra_file);
    free(d->config.bgpd_file);
    free(d->config.ospfd_file);
    free(d->config.ospf6d_file);
    free(d->config.ripd_file);
    free(d->config.ripngd_file);
    free(d->config.isis_file);
}

static void 
change_quagga_config(struct routing_daemon *r, char *cmd, uint8_t proto)
{
    switch (proto) {
        case BGP: {
            send_bgpd_cmd(r->namespace, cmd);
            break;
        }
        case OSPF:{
            send_ospfd_cmd(r->namespace, cmd);
            break;
        }
        case OSPF6: {
            send_ospf6d_cmd(r->namespace, cmd);
            break;
        }
        case RIP:{
            send_ripd_cmd(r->namespace, cmd);
            break;
        }
        case RIPNG:{
            send_ripngd_cmd(r->namespace, cmd);
            break;
        }
        case ISIS:{
            send_isisd_cmd(r->namespace, cmd);
            break;
        }
    } 

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

void 
set_quagga_daemon_isis_file(struct quagga_daemon* d, char *fname)
{
    d->config.isis_file = xmalloc(strlen(fname)+1);
    strcpy(d->config.isis_file, fname);
}