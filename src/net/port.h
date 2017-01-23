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
#define INTERFACE_H value

#include <stdint.h>
#include <uthash/uthash.h>

/* TODO: Move to a global file? */
#define ETH_LEN 6
#define MAX_PORT_NAME 16

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

/** Description of a switch port. 
 *  speed and name fields will not be considered for 
 *  the first version of the simulator. */
 
struct port {
    uint32_t port_id;              /* Identification number.     */
    uint32_t speed;                /* Total speed.               */
    uint32_t curr_speed;           /* Current speed.             */
    uint8_t config;                /* Administrative state.      */
    uint8_t state;                 /* Interface up or down.      */ 
    char name[MAX_PORT_NAME];
    struct port_stats stats;       /* Current port statistics.   */
    uint8_t eth_address[ETH_LEN];  /* Ethernet hardware address. */
    UT_hash_handle hh;             /* Make the struct hashable   */
};

struct port* port_new(uint32_t port_id, uint8_t eth_addr[ETH_LEN]);

#endif