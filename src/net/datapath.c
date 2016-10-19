#include "datapath.h"

static uint32_t current_uuid = 0;

/* Creates a new datapath. 
*
*  A datapath starts without any port assigned.  
*/
struct datapath* 
dp_new(uint64_t dp_id){
    struct datapath * dp = malloc(sizeof(struct datapath));
    dp->uuid = current_uuid;
    dp->dp_id = dp_id;
    dp->ports_num = 0;
    /*TODO: Create/init flow table */
    ++current_uuid;
    return dp;
}
