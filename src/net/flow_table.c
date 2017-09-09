/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */


#include "flow_table.h"
#include "lib/util.h"
#include "lib/openflow.h"
#include <uthash/utlist.h>

static bool is_flow_non_strict_in_mft(struct ofl_flow_key *fmask,
                                      struct mini_flow_table *mft);
static bool match_non_strict(struct ofl_flow_key *key1,
                             struct ofl_flow_key *key1_mask, 
                             struct ofl_flow_key *key2);
static struct flow *match_strict(struct flow *f, struct mini_flow_table *mft,
                                 bool *stop);
/* Creates a new mini flow table.
*
*  A mini flow table needs a flow
*  to be created.
*/
static void
mini_flow_table_new(struct flow_table *ft, struct flow *f)
{
    struct mini_flow_table *mft = xmalloc(sizeof(struct mini_flow_table));
    mft->flows = NULL;
    memcpy(&mft->mask, &f->mask, sizeof(mft->mask));
    HASH_ADD(hh, mft->flows , key, sizeof(struct ofl_flow_key), f);
    DL_APPEND(ft->flows, mft);
}

static void
mini_flow_table_add_flow(struct mini_flow_table *mft,  struct flow *f) {
    HASH_ADD(hh, mft->flows , key, sizeof(struct ofl_flow_key), f);
}

static void
flow_table_del_flow(struct flow_table *ft, struct mini_flow_table *mft,  struct flow *f)
{
    unsigned int num_flows;
    HASH_DEL(mft->flows, f);
    flow_destroy(f);
    num_flows = HASH_COUNT(mft->flows);
    if (!num_flows) {
        DL_DELETE(ft->flows, mft);
        free(mft);
    }
}

static bool
flow_table_del_expired(struct flow_table *ft, struct mini_flow_table *mft, struct flow *f, uint64_t time) {

    bool del = false;
    if (f->hard_timeout) {
        if (f->remove_at < time) {
            del = true;
        }
    }
    else if (f->idle_timeout) {
        uint64_t idle_remove = f->last_used + f->idle_timeout;
        if ( idle_remove < time) {
            del = true;
        }
    }
    if (del) {
        flow_table_del_flow(ft, mft, f);
    }
    return del;
}

struct flow_table*
flow_table_new(uint8_t table_id) {
    struct flow_table *ft =  xmalloc(sizeof(struct flow_table));
    ft->table_id = table_id;
    ft->flows = NULL;
    return ft;
}

void flow_table_destroy(struct flow_table *ft)
{
    struct mini_flow_table *nxt_mft, *tmp;
    DL_FOREACH_SAFE(ft->flows, nxt_mft, tmp) {
        struct flow *cur_flow, *tmp;
        HASH_ITER(hh, nxt_mft->flows, cur_flow, tmp) {
            // flow_printer(cur_flow);
            HASH_DEL(nxt_mft->flows, cur_flow);
            flow_destroy(cur_flow);
        }
        DL_DELETE(ft->flows, nxt_mft);
        free(nxt_mft);
    }
    free(ft);
}

struct flow*
flow_table_lookup(struct flow_table *ft, struct ofl_flow_key *key, uint64_t time)
{
    struct flow *ret_flow = NULL;
    struct mini_flow_table *nxt_mft, *tmp;
    long int cur_priority = -1;
    /* Check all the mini flow tables */
    DL_FOREACH_SAFE(ft->flows, nxt_mft, tmp) {
        struct ofl_flow_key tmp_key;
        struct flow *flow_found;
        /* Cannot modify the flow key, so copy it to a temporary struct. */
        memcpy(&tmp_key, key, sizeof(struct ofl_flow_key));
        /* Apply mini flow table mask to the temporary flow key fields. */
        apply_all_mask(&tmp_key, &nxt_mft->mask);
        /* Search for a match in the mini flow table. */
        HASH_FIND(hh, nxt_mft->flows, &tmp_key,
                  sizeof(struct ofl_flow_key), flow_found);
        if (flow_found) {
            bool expired = flow_table_del_expired(ft, nxt_mft, flow_found,
                                                  time);
            if (!expired) {
                if (flow_found->priority > cur_priority) {
                    /* Take the flow found if priority is
                     *  higher than the last one found. */
                    cur_priority = flow_found->priority;
                    ret_flow = flow_found;
                }
            }
        }
    }
    if (ret_flow) {
        ret_flow->last_used = time;
    }
    return ret_flow;
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
add_flow(struct flow_table *ft, struct flow *f, uint64_t time)
{
    bool added = false;
    struct mini_flow_table *nxt_mft;
    /* Check if a mini flow table for the flow exists.*/
    f->remove_at = f->hard_timeout + time;
    f->last_used = time + f->idle_timeout;
    DL_FOREACH(ft->flows, nxt_mft) {
        /* Masks are equal if a mini flow table exists. */
        if (flow_key_cmp(&f->mask, &nxt_mft->mask)) {
            mini_flow_table_add_flow(nxt_mft, f);
            added = true;
        }
    }
    if (!added) {
        mini_flow_table_new(ft, f);
    }
}

static struct flow
*match_strict(struct flow *f, struct mini_flow_table *mft, bool *stop)
{
    if (flow_key_cmp(&f->mask, &mft->mask)) {
        struct flow *flow_found;
        HASH_FIND(hh, mft->flows, &f->key, sizeof(struct ofl_flow_key), flow_found);
        *stop = true;
        return flow_found;
    }
    return NULL;
}

/* Strict match only matches when table mask == flow_mask (the flow natural mask)*/

/* Modify flow strict:
*   for each flow hash table
*       if table mask == flow_mask
*          if flow in the hash table and equal priority
*               modify flow
*/
static void
mod_flow_strict(struct flow_table *ft, struct flow *f, uint64_t time)
{
    struct mini_flow_table* nxt_mft, *tmp;
    DL_FOREACH_SAFE(ft->flows, nxt_mft, tmp) {
        bool stop = false;
        struct flow *flow_found = match_strict(f, nxt_mft, &stop);
        if (flow_found && flow_found->priority == f->priority &&
                !flow_table_del_expired(ft, nxt_mft, flow_found, time)) {
            flow_add_instructions(flow_found, f->insts);
        }
        if (stop) {
            break;
        }
    }
}

/* Checks if a flow matching is part of the flows in a minitable.
*  It works by applying the mask of the flow over the mask of the minitable.
*  If the resulting mask of the minitable equals the flow mask, it means the
*  flow can be non strictly found.
*/
static bool
is_flow_non_strict_in_mft(struct ofl_flow_key *fmask,
                          struct mini_flow_table *mft)
{
    struct ofl_flow_key tmp_mask;
    memcpy(&tmp_mask, &mft->mask, sizeof(struct ofl_flow_key));
    /* Apply modifying flow mask to temporary flow. */
    apply_all_mask(&tmp_mask, fmask);
    /* Check if the flow can be found in the mini table. */
    return flow_key_cmp(&tmp_mask, fmask);
}

static bool
match_non_strict(struct ofl_flow_key *key1, struct ofl_flow_key *key1_mask,             struct ofl_flow_key *key2) {

    struct ofl_flow_key tmp_key;
    /* Copy  flow key. */
    memcpy(&tmp_key, key2, sizeof(struct ofl_flow_key));
    /* Apply modifying mask to the temporary flow key. */
    apply_all_mask(&tmp_key, key1_mask);
    return flow_key_cmp(&tmp_key, key1);

}

static void
mod_flow_non_strict(struct flow_table *ft, struct flow *f, uint64_t time)
{
    struct mini_flow_table *nxt_mft, *tmp;
    DL_FOREACH_SAFE(ft->flows, nxt_mft, tmp) {
        if (is_flow_non_strict_in_mft(&f->mask, nxt_mft)) {
            struct flow *cur_flow;
            for (cur_flow = nxt_mft->flows; cur_flow != NULL;
                    cur_flow = cur_flow->hh.next) {
                if (match_non_strict(&f->key, &f->mask, &cur_flow->key) &&
                        !flow_table_del_expired(ft, nxt_mft, cur_flow, time)) {
                    /* Replace instructions. */
                    flow_add_instructions(cur_flow, f->insts);
                }
            }
        }
    }
}

void
modify_flow(struct flow_table *ft, struct flow *f,
            bool strict, uint64_t time)
{
    if (strict) {
        mod_flow_strict(ft, f, time);
    }
    else {
        mod_flow_non_strict(ft, f, time);
    }
}

void
flow_table_stats(struct flow_table *ft, struct ofl_flow_stats_req *req,
                 struct flow ***flows, size_t *flow_count, uint64_t time)
{
    size_t allocated = 1;
    struct mini_flow_table *nxt_mft, *tmp;
    DL_FOREACH_SAFE(ft->flows, nxt_mft, tmp) {
        if (is_flow_non_strict_in_mft(&req->mask, nxt_mft)) {
            struct flow *cur_flow;
            for (cur_flow = nxt_mft->flows; cur_flow != NULL;
                    cur_flow = cur_flow->hh.next) {
                if ( /*(req->out_port == OFPP_ANY ||
                        flow_entry_has_out_port(cur_flow)) && */
                        match_non_strict(&req->match, &req->mask, &cur_flow->key) &&
                        !flow_table_del_expired(ft, nxt_mft, cur_flow, time)) {
                    /* Increase size if needed. */
                    if ((allocated) == (*flow_count)) {
                        (*flows) = xrealloc(*flows, (sizeof(struct flows *)) * (allocated) * 2);
                        allocated *= 2;
                    }
                    (*flows)[(*flow_count)] = cur_flow;
                    (*flow_count)++;
                }
            }
        }
    }
}

/* Delete flow strict:
*   found = 0
*   for each flow hash table
*       if table mask == flow_mask
*          if flow in the hash table and equal priority
*               delete flow
                found = 1
        stop in the case of search in the correct mini flow table.
*   if not found
*      return error
*
*/
static void
del_flow_strict(struct flow_table *ft, struct flow *f, uint64_t time)
{
    struct mini_flow_table* nxt_mft, *tmp;
    DL_FOREACH_SAFE(ft->flows, nxt_mft, tmp) {
        bool stop = false;
        struct flow *flow_found = match_strict(f, nxt_mft, &stop);
        if (flow_found && flow_found->priority == f->priority &&
                !flow_table_del_expired(ft, nxt_mft, flow_found, time)) {
            flow_table_del_flow(ft, nxt_mft, flow_found);
        }
        if (stop) {
            break;
        }
    }
}


static void
del_flow_non_strict(struct flow_table *ft, struct flow *f, uint64_t time)
{
    struct mini_flow_table *nxt_mft, *tmp;
    DL_FOREACH_SAFE(ft->flows, nxt_mft, tmp) {
        if (is_flow_non_strict_in_mft(&f->mask, nxt_mft)) {
            struct flow *cur_flow;
            for (cur_flow = nxt_mft->flows; cur_flow != NULL;
                    cur_flow = cur_flow->hh.next) {
                if (match_non_strict(&f->key, &f->mask, &cur_flow->key) &&
                        !flow_table_del_expired(ft, nxt_mft, cur_flow, time)) {
                    /* Delete flow. */
                    flow_table_del_flow(ft, nxt_mft, cur_flow);
                }
            }
        }
    }
}

void delete_flow(struct flow_table *ft, struct flow *f, uint64_t time,
                 bool strict)
{
    if (strict) {
        del_flow_strict(ft, f, time);
    }
    else {
        del_flow_non_strict(ft, f, time);
    }
}
