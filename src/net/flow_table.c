#include "flow_table.h"
#include <uthash/utlist.h>
#include <stdio.h>
/* Creates a new mini flow table. 
*   
*  A mini flow table needs a flow
*  to be created.
*/
static void
mini_flow_table_new(struct flow_table *ft, struct flow *f)
{
    struct mini_flow_table *mft = malloc(sizeof(struct mini_flow_table));
    mft->flows = NULL;
    memcpy(&mft->mask, &f->mask, sizeof(mft->mask));
    HASH_ADD(hh, mft->flows , key, sizeof(struct flow_key), f);
    DL_APPEND(ft->flows, mft);
}

static void
mini_flow_table_add_flow(struct mini_flow_table* mft,  struct flow *f){
    HASH_ADD(hh, mft->flows , key, sizeof(struct flow_key), f);
}

struct flow_table* 
flow_table_new(void){
    struct flow_table * ft = malloc(sizeof(struct flow_table));
    ft->flows = NULL;
    return ft;
}

/* Insert flow:
*  added = False  
*  for each flow hash table
*      if table mask == flow_mask
*          Insert in the hash table
*          added = True
*          break
*   if not added
*      Create a new hash table
*      Insert the flow in the hash table
*/
void 
add_flow(struct flow_table* ft, struct flow* f)
{
    bool added = false;
    struct mini_flow_table* nxt_mft;
    DL_FOREACH(ft->flows, nxt_mft){
        if (flow_key_cmp(&f->mask, &nxt_mft->mask)){
            mini_flow_table_add_flow(nxt_mft, f);
            added = true;    
        }
    } 
    if (!added){
        mini_flow_table_new(ft, f);
    }
}

/* Strict match only matches when table mask = flow_mask (the flow natural mask)*/

/* Modify flow strict:
*   for each flow hash table   
*       if table mask == flow_mask 
*          if flow in the hash table and equal priority
*               modify flow
*/

/* Delete flow strict:
*   found = 0
*   for each flow hash table   
*       if table mask == flow_mask 
*          if flow in the hash table and equal priority
*               delete flow
                found = 1
*   if not found
*      return error
*/

/* For non strict match, we need to apply the table mask, 
    similarly to packet matching (flow do not have natural mask, it goes through every table applying the table mask and checking if the match happens)*/

/* Modify flow non strict(mod_flow):
*   
*   for each flow hash table   
*       apply flow hash table mask to mod_flow  
*          if flow in the hash table 
*               modify flow
*/

/* Delete flow non strict(mod_flow):
*   
*   for each flow hash table   
*       apply flow hash table mask to del_flow  
*          if flow in the hash table 
*               delete flow
*/