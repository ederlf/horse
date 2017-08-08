#ifndef OF_PACK_H
#define OF_PACK_H 1

#include "netflow.h"
#include "sim_event.h"

#define MAX_PACKET_IN_DATA 128

uint8_t *of_packet_in(struct netflow *f, size_t *len);

#endif