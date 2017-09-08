#ifndef OF_UNPACK_H
#define OF_UNPACK_H 1

#include "flow.h"
#include "netflow.h"
#include "action_list.h"
#include <loci/loci.h>

/* Unpack is actually just copying loci's match fields to flow key 
 * In the future , consider using the loci structure in the flow 
 */ 
void unpack_match_fields(of_match_fields_t *fields, struct ofl_flow_key *key);
void unpack_instructions(of_list_instruction_t *insts, 
                         struct instruction_set *is);
/* Will use only the flow_mod_modify_t from loci object */
int unpack_flow_mod(of_object_t *obj, struct flow *f);
int unpack_flow_stats_request(of_object_t *obj, 
                              struct ofl_flow_stats_req *req);
int unpack_packet_out(of_object_t *obj, struct netflow* f, 
                   struct action_list *al);

#endif