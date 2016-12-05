/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "datapath.h"
#include "lib/util.h" 

static uint32_t current_uuid = 0;

/* Creates a new datapath. 
*
*  A datapath starts without any port assigned.  
*/
struct datapath* 
dp_new(uint64_t dp_id){
    struct datapath *dp = xmalloc(sizeof(struct datapath));
    dp->uuid = current_uuid;
    dp->dp_id = dp_id;
    dp->ports_num = 0;
    /*TODO: Create/init flow table */
    ++current_uuid;
    return dp;
}
