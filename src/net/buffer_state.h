#ifndef BUFFER_STATE
#define BUFFER_STATE 1

#include "lib/netflow.h"

struct buffer_state {
	int capacity;  /* Maximum amount of bytes it can 
                            store before dropping*/
    int load;	    /* How full is the buffer */
    // uint32_t avg_queue_delay; /* TODO */
    uint64_t last_updated;
};

void buffer_state_init(struct buffer_state *ts);

void buffer_state_update_capacity(struct buffer_state *ts, int bits);

int buffer_state_calculate_loss(struct buffer_state *ts, uint32_t bw);

#endif