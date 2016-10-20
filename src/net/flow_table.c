#include "flow_table.h"

/* Insert flow:
*  added = 0  
*  for each flow hash table
*      if table mask == flow_mask
*          Insert in the hash table
*          added = 1
*          break
*   if not added
*      Create a new hash table
*      Insert the flow in the hash table
*/

/* Strict match only matches when table mask = flow_mask (the flow natural mask)

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