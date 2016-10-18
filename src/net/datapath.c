#include "datapath.h"

struct datapath* 
dp_new(uint64_t dp_id){
    struct datapath * dp = malloc(sizeof(struct datapath));
    dp->dp_id = dp_id;
    dp->ports_num = 0;
    /*TODO: Create/init flow table */
    return dp;
}
