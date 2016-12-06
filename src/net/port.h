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

/* TODO: Move to a global file? */
#define ETH_LEN 6
#define MAX_PORT_NAME 16

enum port_config {
    PORT_DOWN = 0,     /* Administratively down.   */
    PORT_NO_RECV = 1,  /* Drop all flows received. */
    PORT_NO_FWD = 2    /* Drop all flows output.   */
};

enum port_state {
    PORT_LINK_DOWN = 0, /* No connectivity. */
    PORT_BLOCKED = 1,   
    PORT_LIVE = 2
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

/** Description of a switch port. 
 *  
 */
struct port {
    uint32_t id;                   /* Identification number.     */
    uint32_t speed;                /* Total speed.               */
    uint32_t curr_speed;           /* Current speed.             */
    uint8_t config;                /* Administrative state.      */
    uint8_t state;                 /* Interface up or down.      */ 
    char name[MAX_PORT_NAME];
    struct port_stats stats;       /* Current port statistics.   */
    uint8_t eth_address[ETH_LEN];  /* Ethernet hardware address. */
};

/*Port operations: create, send, receive, update counters */
void port_init(struct port *p);
void port_send(struct port p);
void port_receive(struct port p);

#endif