#include "buffer_state.h"

void 
buffer_state_init(struct buffer_state *ts)
{
    memset(ts, 0x0, sizeof(struct buffer_state));
}

void 
buffer_state_update_capacity(struct buffer_state *ts, int bits)
{
    int load = ts->load + bits;
    if (load < 0){
      ts->load = 0;
    }
    else {
      ts->load = load;
    }
}

int buffer_state_calculate_loss(struct buffer_state *ts, uint32_t bw)
{
  int total = ts->capacity + bw;
  if(ts->load > total) {
    return ts->load - total;
  }
  return 0;
}
