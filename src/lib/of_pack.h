#ifndef OF_PACK_H
#define OF_PACK_H 1

#include "sim_event.h"

#define MAX_PACKET_IN_DATA 128

uint8_t *of_packet_in(struct sim_event *ev);

#endif