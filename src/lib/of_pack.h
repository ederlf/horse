#ifndef OF_PACK_H
#define OF_PACK_H 1

#include "sim_event.h"
#include <loci/loci.h>

#define MAX_PACKET_IN_DATA 128

uint8_t *of_pack(of_object_t *msg, uint8_t type, size_t *len);

uint8_t *of_packet_in(struct sim_event *ev, size_t *len);

#endif