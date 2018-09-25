/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef INTERFACE_H
#define INTERFACE_H 1

#include "lib/packets.h"
#include "buffer_state.h"
#include <stdint.h>
#include <uthash/uthash.h>

#define MAX_PORT_NAME 64

enum port_config {
    PORT_DOWN = 1 << 0,     /* Administratively down.   */
    PORT_UP = 1 << 1,     /* Administratively up.   */
    PORT_NO_RECV = 1 << 2,  /* Drop all flows received. */
    PORT_NO_FWD = 1 << 3    /* Drop all flows output.   */
};

enum port_state {
    PORT_LINK_DOWN = 1 << 0, /* No connectivity. */
    PORT_BLOCKED = 1 << 1,   
    PORT_LIVE = 1 << 2
};

/**  
 *  Port statistics for the number of 
 *  received/transmitted packets/bytes.
 */
struct port_stats{
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
};

/* The other side of a port */ 
struct end_point {
    uint64_t dp_id;
    uint32_t port_id;
};

struct ipv4_info {
    uint32_t addr;
    uint32_t netmask;
    uint32_t bcast_addr; 
};

struct ipv6_info {
    uint8_t addr[IPV6_LEN];
    uint8_t netmask[IPV6_LEN];
};

/** Description of a switch port. 
 *  speed and name fields will not be considered for 
 *  the first version of the simulator. */
 
struct port {
    uint32_t port_id;              /* Identification number.        */
    uint32_t peer;                 /* Features advertised by peer   */ 
    uint32_t max_speed;            /* Total speed in kbps.      */
    uint32_t curr;                 /* Current features              */
    uint32_t curr_speed;           /* Current speed in kbps.        */
    uint32_t advertised;           /* Features currently advertised */
    uint32_t supported;            /* Features supported by the port.*/
    uint8_t config;                /* Administrative state.          */
    uint8_t state;                 /* Interface up or down.          */ 
    char name[MAX_PORT_NAME];
    struct port_stats stats;       /* Current port statistics.       */
    struct port_stats prev_stats;  /* Used to calculate the rate     */ 
    uint8_t eth_address[ETH_LEN];  /* Ethernet hardware address.     */
    struct ipv4_info *ipv4_addr;   /* Port may have an IPv4 assigned */ 
    struct ipv6_info *ipv6_addr;   /* Port may have an IPv6 assigned */
    struct buffer_state buffer_state;
    UT_hash_handle hh;             /* Make the struct hashable       */
};

struct port* port_new(uint32_t port_id, uint8_t eth_addr[ETH_LEN], uint32_t speed, uint32_t curr_speed);
void port_add_v4addr(struct port *p, uint32_t ipv4_addr, uint32_t netmask);
void port_add_v6addr(struct port *p, uint8_t ipv6_addr[IPV6_LEN], uint8_t netmask[IPV6_LEN]);
void port_set_name(struct port *p, char *name);

#endif