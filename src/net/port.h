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
    uint32_t id; /* Identification number*/
    uint8_t eth_address[ETH_LEN]; /* Ethernet hardware address*/
    uint8_t state; /* Interface up or down */ 
    uint32_t speed; /* Total speed */
    uint32_t curr_speed; /* Current speed */
    struct port_stats stats; /* Current port statistics */
    char name[MAX_PORT_NAME];
};

/*Port operations: create, send, receive, update counters */

// struct port * create_port(){


// }

/* TODO: Create error code? */
// void recv_flow(){


// }

// void send_flow(){

    
// }

#endif