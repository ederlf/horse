#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pcap/pcap.h>
#include <pthread.h>
#include <sys/socket.h> 
#include "routing_msg/routing_msg.h"
#include "lib/packets.h"

struct daemon_data {
    int server_fd;
    uint32_t router_id;
};

struct pcap_data {
    pcap_t *pd;
    struct daemon_data ddata;
};  

#define PORT 6000    /* the port client will be connecting to */
#define MAXDATASIZE 100 /* max number of bytes we can get at once */
#define ERR_RET(x) do { perror(x); return EXIT_FAILURE; } while (0);
#define BUFFER_SIZE 4095

static 
void daemonise(uint32_t rid) {
    // Fork, allowing the parent process to terminate.
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        _exit(0);
    }

    // Start a new session for the daemon.
    if (setsid()==-1) {
        fprintf(stderr, "failed to become a session leader while daemonising(errno=%d)",errno);
    }

    // Fork again, allowing the parent process to terminate.
    signal(SIGHUP,SIG_IGN);
    pid=fork();
    if (pid == -1) {
        fprintf(stderr, "failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        FILE *pid_file;
        char rname[60];
        sprintf(rname, "/tmp/daemon%u.pid", ntohl(rid));
        pid_file = fopen(rname, "w");
        fprintf(pid_file, "%u\n", pid);
        fclose(pid_file);
        _exit(0);
    }

    // Set the current working directory to the root directory.
    if (chdir("/") == -1) {
        fprintf(stderr, "failed to change working directory while daemonising (errno=%d)",errno);
    }

    // Set the user file creation mask to zero.
    umask(0);

    // Close then reopen standard file descriptors.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    if (open("/dev/null",O_RDONLY) == -1) {
        fprintf(stderr, "failed to reopen stdin while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_WRONLY) == -1) {
        fprintf(stderr, "failed to reopen stdout while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_RDWR) == -1) {
        fprintf(stderr, "failed to reopen stderr while daemonising (errno=%d)",errno);
    }
}

int client_connect(uint32_t server_ip, uint16_t port)
{
    int sockfd; // numbytes;  
    // char buf[MAXDATASIZE];
    struct sockaddr_in their_addr; /* connector's address information */

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      /* host byte order */
    their_addr.sin_port = htons(port);    /* short, network byte order */
    their_addr.sin_addr.s_addr = server_ip;
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

    if (connect(sockfd, (struct sockaddr *)&their_addr, \
                                          sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    return sockfd;
    return 0;
}


static void 
send_activity(int fd, uint32_t router_id)
{
    uint8_t msg_data[HEADER_LEN] = {0x0, ROUTER_ACTIVITY, 0x0, HEADER_LEN};
    memcpy(msg_data + 4, &router_id, sizeof(uint32_t));
    int ret = send(fd, msg_data, HEADER_LEN, 0);
    if (ret == -1){
        perror("Failed to send message");
    }
}

/* callback function when a packet is captured */
static void capture_cb(u_char *user,
    const struct pcap_pkthdr *h, const u_char *packet)
{
    struct pcap_data *data = (struct pcap_data*) user;
    (void) h;
    uint8_t *tmp_pkt = (uint8_t*) packet;
    uint16_t eth_type = *((uint16_t*) (tmp_pkt+14));
    if (ntohs(eth_type) == ETH_TYPE_IP){
        tmp_pkt += 16;
        struct ip_header *ip = (struct ip_header*) tmp_pkt;
        if (ip->ip_proto == IP_PROTO_OSPF) {
            tmp_pkt += sizeof(struct ip_header);
            uint8_t ospf_type = *(tmp_pkt+1);
            /* Only send activities to messages other than OSPF Hello */
            if (ospf_type > 1){
                send_activity(data->ddata.server_fd, data->ddata.router_id);
            }
        } 
        else if (ip->ip_proto == IP_PROTO_TCP){
            tmp_pkt += sizeof(struct ip_header);
            struct tcp_header *tcp= (struct tcp_header*) tmp_pkt;
            /* BGP */
            uint32_t hlen = (ntohs(tcp->tcp_ctl) >> 12) * 4;
            if (ntohs(tcp->tcp_src) == TCP_PORT_BGP || 
                ntohs(tcp->tcp_dst) == TCP_PORT_BGP){
                /* Skip marker and length */
                tmp_pkt += hlen + 18;
                uint8_t bgp_type = *tmp_pkt;
                /* SEND Activity to Controller */
                if (bgp_type == BGP_UPDATE || bgp_type == BGP_OPEN){
                    send_activity(data->ddata.server_fd,
                                  data->ddata.router_id);
                }
            }
        }
    }
}

static void* capture(void* args)
{
    /* the error code buf of libpcap, PCAP_ERRBUF_SIZE = 256 */
    char ebuf[PCAP_ERRBUF_SIZE];
    pcap_t *pd;
    int pcap_loop_ret;
    struct bpf_program filter;
    struct pcap_data data;
    struct daemon_data *dd = (struct daemon_data*) args;
    /* grab a device to peak into */

    /* create capture handler of libpcap */
    pd = pcap_open_live("any", 1024, 0, 1000, ebuf);
    if(pd == NULL) {
        /* e.g. "Operation not permitted" */
        printf("pcap_open_live error: %s\n", ebuf);
        exit(1);
    }
    data.pd = pd;
    data.ddata = *dd;
    if ( pcap_compile(pd, &filter, "ip", 1, PCAP_NETMASK_UNKNOWN) == -1){
       fprintf(stderr, "ERROR: %s\n", pcap_geterr(pd) );
       exit(1);
    }

    /* Load the filter program into the packet capture device. */
    if (pcap_setfilter(pd,&filter) == -1){
        fprintf(stderr, "ERROR: %s\n", pcap_geterr(pd) );
        exit(1);
    }

    /* start the loop of capture, loop 5 times */
    printf("Starting loop\n");

    pcap_loop_ret = pcap_loop(pd, -1, capture_cb, (u_char*)&data);
    printf("pcap_loop returned: %d\n", pcap_loop_ret);

    pcap_close(pd);
    return 0;
}

static void
send_next_hop(int fd, uint32_t router_id, uint32_t destination,
              uint32_t netmask, uint32_t next_hop)
{
    uint8_t msg_data[HEADER_LEN+16] = {0x0, ROUTER_FIB, 0x0, HEADER_LEN+12};
    memcpy(msg_data+4, &router_id, sizeof(uint32_t));
    memcpy(msg_data + HEADER_LEN, &destination, sizeof(uint32_t));
    memcpy(msg_data + 12, &netmask, sizeof(uint32_t));
    memcpy(msg_data + 16, &next_hop, sizeof(uint32_t));
    memset(msg_data + 20, 0x0, sizeof(uint32_t));
    int ret = send(fd, msg_data,
                   HEADER_LEN+16, 0);
    if (ret == -1){
        perror("Failed to send message");
    }
}

static
int loop (int sock, struct sockaddr_nl *addr, struct daemon_data *dd)
{
    int     received_bytes = 0;
    struct  nlmsghdr *nlh;
    char    destination_address[32];
    unsigned char    route_netmask = 0;
    unsigned char    route_protocol = 0;
    char    gateway_address[32];
    struct  rtmsg *route_entry;  /* This struct represent a route entry \
                                    in the routing table */
    struct  rtattr *route_attribute; /* This struct contain route \
                                            attributes (route type) */
    int     route_attribute_len = 0;
    char    buffer[BUFFER_SIZE];

    bzero(destination_address, sizeof(destination_address));
    bzero(gateway_address, sizeof(gateway_address));
    bzero(buffer, sizeof(buffer));

    /* Receiving netlink socket data */
    while (1)
    {
        received_bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (received_bytes < 0)
            ERR_RET("recv");
        /* cast the received buffer */
        nlh = (struct nlmsghdr *) buffer;
        /* If we received all data ---> break */
        if (nlh->nlmsg_type == NLMSG_DONE)
            break;
        /* We are just intrested in Routing information */
        if (addr->nl_groups == RTMGRP_IPV4_ROUTE)
            break;
    }

    /* Reading netlink socket data */
    /* Loop through all entries */
    /* For more informations on some functions :
     * http://www.kernel.org/doc/man-pages/online/pages/man3/netlink.3.html
     * http://www.kernel.org/doc/man-pages/online/pages/man7/rtnetlink.7.html
     */

    for ( ; NLMSG_OK(nlh, received_bytes); \
                    nlh = NLMSG_NEXT(nlh, received_bytes))
    {
        uint32_t destination = 0;
        uint32_t netmask= 0;
        uint32_t next_hop= 0;
        struct rtnexthop *mhops = NULL;
        int attrlen = 0;
        /* Get the route data */
         route_entry = (struct rtmsg *) NLMSG_DATA(nlh);

        /* We are just interested in main routing table */
                /*
        if (route_entry->rtm_table != RT_TABLE_MAIN)
            continue;
                */

                /*
        if (route_entry->rtm_protocol != RTPROT_UNSPEC)
            continue;
                */

        route_netmask = route_entry->rtm_dst_len;
        netmask = htonl(~((1 << (32 - route_netmask)) - 1));
        route_protocol = route_entry->rtm_protocol;

        /* Get attributes of route_entry */
        route_attribute = (struct rtattr *) RTM_RTA(route_entry);

        /* Get the route atttibutes len */
        route_attribute_len = RTM_PAYLOAD(nlh);
        /* Loop through all attributes */
        
        for ( ; RTA_OK(route_attribute, route_attribute_len); \
            route_attribute = RTA_NEXT(route_attribute, route_attribute_len))
        {

            /* Get the destination address */
            if (route_attribute->rta_type == RTA_DST)
            {
                destination = *(uint32_t*) RTA_DATA(route_attribute);
            }
            /* Get the gateway (Next hop) */
            if (route_attribute->rta_type == RTA_GATEWAY)
            {   
                
                next_hop = *(uint32_t*) RTA_DATA(route_attribute);
                
            }

            if (route_attribute->rta_type == RTA_MULTIPATH)
            {
                mhops = (struct rtnexthop*) RTA_DATA(route_attribute);
                int rtnhp_len = RTA_PAYLOAD(route_attribute);
                if (rtnhp_len < (int) sizeof(*mhops) || 
                    mhops->rtnh_len > rtnhp_len){
                    continue;
                }
                attrlen = rtnhp_len - sizeof(struct rtnexthop);
            }
        }
        /* TODO: Send all prefixes in a single message? */
        /* Now we can dump the routing attributes */
        if (nlh->nlmsg_type == RTM_DELROUTE)
            fprintf(stderr, "Deleting route to destination --> %s/%d proto %d and gateway %s\n", \
                destination_address, route_netmask, route_protocol, gateway_address);
        
        if (nlh->nlmsg_type == RTM_NEWROUTE){
            if (destination) {
                if (mhops){
                    if (attrlen) {
                        struct rtattr *attr = RTNH_DATA(mhops);
                        for (; RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen)) {
                            if (attr->rta_type == RTA_GATEWAY) {
                                uint32_t nh = *(uint32_t*) RTA_DATA(attr);
                                send_next_hop(dd->server_fd, dd->router_id,
                                             destination, netmask, nh);  
                            }
                        }
                    }
                }

                if (next_hop){
                    send_next_hop(dd->server_fd, dd->router_id,
                                  destination, netmask, next_hop);
                }
            }
            fprintf(stderr, "Adding route to destination --> %s/%d proto %d and gateway %s\n", \
                                destination_address, route_netmask, route_protocol, gateway_address);
        }
    }
        
    return 0;
}

static 
void* route_table_monitor(void* args)
{
    int sock = -1;
    struct sockaddr_nl addr;
    struct daemon_data *dd = (struct daemon_data*) args;
    /* Zeroing addr */
    bzero (&addr, sizeof(addr));

    if ((sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0){
       perror("Netlink socket creation error");
       exit(1);
    }

    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_ROUTE;

    if (bind(sock,(struct sockaddr *)&addr,sizeof(addr)) < 0){
        perror("Netlink socket binding error");
        exit(1);
    }

    while (1){
        loop (sock, &addr, dd);
    }

    /* Close socket */
    close(sock);

    return 0;
} 

int main(int argc, char* argv[])
{
    pthread_t pcap;
    pthread_t route_mon;
    struct daemon_data dd;
    if (argc < 2) {
        fprintf(stderr, "Missing router-id argument. " 
            "e.g: ./horse_daemon 127.0.0.1\n");
        exit(0);
    }
    uint32_t router_id;
    uint32_t server_ip; 
    inet_pton(AF_INET, argv[1], &router_id);
    inet_pton(AF_INET, "172.20.254.254", &server_ip);
    dd.router_id = router_id;
    dd.server_fd = client_connect(server_ip, 6000);
    daemonise(router_id);
    if (pthread_create(&pcap, NULL, capture, &dd) != 0) {
        perror("pthread_create() error");
        exit(1);
    }
    if (pthread_create(&route_mon, NULL, route_table_monitor, &dd) != 0) {
        perror("pthread_create() error");
        exit(1);
    }
    pthread_join(pcap, 0);
    pthread_join(route_mon, 0);
    // close(sockfd);
    return 0;
}

