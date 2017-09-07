/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */


#ifndef FLOW_TABLE_H
#define FLOW_TABLE_H 1

#include "lib/flow.h"
#include <inttypes.h>


/* Mini flow tables are hash maps that 
*  contain only one combination of match 
*  fields. 
*
*  In the struct, flows is the hash 
*  table of flows and mask indicates 
*  the active fields of the table. 
**/
struct mini_flow_table {
    struct flow* flows;
    struct ofl_flow_key mask;
    struct mini_flow_table *prev; /* needed for a doubly-linked list only */
    struct mini_flow_table *next; /* needed for singly- or doubly-linked lists */
};

typedef struct flow* flow_hash_list;

struct flow_table {
    struct mini_flow_table *flows; /*List of flows hash maps*/
    uint8_t table_id;
};

struct flow_table* flow_table_new(uint8_t table_id);
void flow_table_destroy(struct flow_table *ft);
struct flow* flow_table_lookup(struct flow_table* ft, struct ofl_flow_key *key, uint64_t time);
void add_flow(struct flow_table *ft, struct flow *f, uint64_t time);
void modify_flow(struct flow_table *ft, struct flow *f, bool strict, uint64_t time);
void delete_flow(struct flow_table *ft, struct flow *f, bool strict);


#endif /* FLOW_TABLE_H */ 