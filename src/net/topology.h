/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef TOPOLOGY_H
#define TOPOLOGY_H 1

#include "datapath.h"
#include "lib/util.h"

/* Total number of possible datapaths
*  UINT32_MAX == 2 ^ 16 datapaths 
*/
#define MAX_DPS UINT16_MAX 

/* Representation of a link of the network. 
*   
*/
struct link {
    uint32_t conn_dp;   /* Id of the datapath connected.  */
    uint32_t latency;   /* Latency of the link in ms.     */
    uint32_t bandwidth; /* Maximum bandwidth of the link. */
    struct link* next;  /* Next link in a list.           */
};

/* Represents the network topology */
struct topology {
    struct datapath *dps;           /* Hash map of switches. 
                                     * The datapath id is the key. */
    struct link* links[MAX_DPS];    /* List of link edges of the topology. */
    uint32_t degree[MAX_DPS];       /* number of links connected to dps. */ 
    uint32_t ndatapaths;             /* Number of datapaths. */
    uint32_t nlinks;                 /* Number of links. */
};

void topology_add_link(struct topology *t, uint64_t dpidA, uint64_t dpidB, uint32_t bw, uint32_t latency);
void topology_init(struct topology* topo);
void topology_add_switch(struct topology *topo, struct datapath *dp);
void topology_destroy(struct topology *topo);


#endif /* TOPOLOGY_H */