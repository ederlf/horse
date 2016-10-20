#ifndef FLOW_TABLE_H
#define FLOW_TABLE_H 1

#include "lib/flow.h"
#include <inttypes.h>

/* Total number of tables per datapath */
#define MAX_TABLES 128

struct flow_table {
    struct flow *flows[MAX_TABLES];
};



#endif /* FLOW_TABLE_H */ 