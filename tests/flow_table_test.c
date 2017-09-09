#include "net/flow_table.h"
#include <uthash/utlist.h>
#include "cmockery_horse.h"

void lookup(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    set_in_port(fl, 1);
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_eth_type(fl2, 0x806);
    add_flow(ft, fl2, 0);
    struct ofl_flow_key key;
    memset(&key, 0x0, sizeof(struct ofl_flow_key));
    key.eth_type = 0x800;
    key.in_port = 1;
    ret = flow_table_lookup(ft, &key, 0);
    assert_int_not_equal(ret, NULL);
    assert_int_equal(ret->key.eth_type, 0x800);
    flow_table_destroy(ft);
}

void lookup_expired(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    fl->hard_timeout = 5;
    add_flow(ft, fl, 0);
    struct ofl_flow_key key;
    memset(&key, 0x0, sizeof(struct ofl_flow_key));
    key.eth_type = 0x800;
    ret = flow_table_lookup(ft, &key, 4);
    assert_int_not_equal(ret, NULL);
    assert_int_equal(ret->key.eth_type, 0x800);
    ret = flow_table_lookup(ft, &key, 10);
    assert_int_equal(ret, NULL);
    flow_table_destroy(ft);
}

void stress_lookup_expired(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    int i;
    for (i = 0; i < 1000000; ++i) {
        struct flow *fl = flow_new();
        set_eth_type(fl, 0x800);
        fl->hard_timeout = 5;
        add_flow(ft, fl, 0);
        struct ofl_flow_key key;
        memset(&key, 0x0, sizeof(struct ofl_flow_key));
        key.eth_type = 0x800;
        ret = flow_table_lookup(ft, &key, 4);
        assert_int_not_equal(ret, NULL);
        assert_int_equal(ret->key.eth_type, 0x800);
        ret = flow_table_lookup(ft, &key, 10);
        assert_int_equal(ret, NULL);
    }
    flow_table_destroy(ft);
}

/* Must return the flow with highest priority */
void lookup_priority(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    set_in_port(fl, 1);
    fl->priority = 10;
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_eth_type(fl2, 0x800);
    fl2->priority = 1;
    add_flow(ft, fl2, 0);
    struct ofl_flow_key key;
    memset(&key, 0x0, sizeof(struct ofl_flow_key));
    key.eth_type = 0x800;
    key.in_port = 1;
    ret = flow_table_lookup(ft, &key, 0);
    if (ret) {
        assert_int_equal(ret->key.in_port, 1);
    }
    flow_table_destroy(ft);
}

/* Test succeed if flow can be found in the table */
void add_single_flow(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    add_flow(ft, fl, 0);
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        unsigned int num_flows = HASH_COUNT(hash);
        assert_int_equal(num_flows, 1);
        HASH_FIND(hh, hash, &fl->key, sizeof(struct ofl_flow_key), ret);
        if (ret) {
            /* Key was checked in the hash find...*/
            assert_int_equal(ret->key.eth_type, 0x800);
        }
    }
    flow_table_destroy(ft);
}

/* Test succeeds if there is ONE mini flow table and TWO flows */
void add_flows_same_field_type(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_eth_type(fl2, 0x806);
    add_flow(ft, fl2, 0);
    int count;
    DL_COUNT(ft->flows, elt, count);
    assert_int_equal(count, 1);
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        unsigned int num_flows = HASH_COUNT(hash);
        assert_int_equal(num_flows, 2);
    }
    flow_table_destroy(ft);
}

/*  Test succeeds if there are TWO mini flow tables
*   and ONE flow in each mini table.
*/
void add_flows_diff_field_type(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    set_ip_proto(fl, 17);
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_eth_type(fl2, 0x806);
    add_flow(ft, fl2, 0);
    int count;
    DL_COUNT(ft->flows, elt, count);
    assert_int_equal(count, 2);
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        unsigned int num_flows = HASH_COUNT(hash);
        assert_int_equal(num_flows, 1);
    }
    flow_table_destroy(ft);
}

/* Succeed if instruction for a single flow is modified */
void modify_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    struct instruction_set is;
    instruction_set_init(&is);
    struct goto_table gt;
    inst_goto_table(&gt, 1);
    struct write_metadata wm;
    inst_write_metadata(&wm, 0xbeef, 0xffffffffffffffff);
    add_goto_table(&is, gt);
    add_write_metadata(&is, wm);
    /* Match */
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    flow_add_instructions(fl, is);
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_ipv4_dst(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    flow_add_instructions(fl2, is);
    add_flow(ft, fl2, 0);
    /* Flow to be modified */
    struct flow *fl3 = flow_new();
    struct instruction_set is2;
    instruction_set_init(&is2);
    struct clear_actions clear;
    inst_clear_actions(&clear);
    add_clear_actions(&is2, clear);
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    flow_add_instructions(fl3, is2);
    modify_flow(ft, fl3, true, 0);
    /* Check if only the first flow is modified*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct ofl_flow_key), ret);
        if (ret) {
            assert_int_equal(ret->insts.active, is2.active);
        }
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct ofl_flow_key), ret);
        if (ret) {
            assert_int_equal(ret->insts.active, is.active);
        }
    }
    flow_destroy(fl3);
    flow_table_destroy(ft);
}

/* The flow should not be found */
void modify_strict_expired(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    fl->hard_timeout = 5;
    struct instruction_set is;
    instruction_set_init(&is);
    struct goto_table gt;
    inst_goto_table(&gt, 1);
    struct write_metadata wm;
    inst_write_metadata(&wm, 0xbeef, 0xffffffffffffffff);
    add_goto_table(&is, gt);
    add_write_metadata(&is, wm);
    /* Match */
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    flow_add_instructions(fl, is);
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_ipv4_dst(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    flow_add_instructions(fl2, is);
    add_flow(ft, fl2, 0);
    /* Flow to be modified */
    struct flow *fl3 = flow_new();
    struct instruction_set is2;
    instruction_set_init(&is2);
    struct clear_actions clear;
    inst_clear_actions(&clear);
    add_clear_actions(&is2, clear);
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    flow_add_instructions(fl3, is2);
    modify_flow(ft, fl3, true, 10);
    /* Check if only the first flow is deleted because of expiration*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct ofl_flow_key), ret);
        assert_int_equal(ret, NULL);
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct ofl_flow_key), ret);
        if (ret) {
            assert_int_equal(ret->insts.active, is.active);
        }
    }
    flow_destroy(fl3);
    flow_table_destroy(ft);
}

/* Succeed if instruction for two flows is modified */
void modify_non_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    struct instruction_set is;
    instruction_set_init(&is);
    struct goto_table gt;
    inst_goto_table(&gt, 1);
    struct write_metadata wm;
    inst_write_metadata(&wm, 0xbeef, 0xffffffffffffffff);
    add_goto_table(&is, gt);
    add_write_metadata(&is, wm);
    /* Match */
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    flow_add_instructions(fl, is);
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_ip_ecn(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    flow_add_instructions(fl2, is);
    add_flow(ft, fl2, 0);
    /* Flow to be modified */
    struct flow *fl3 = flow_new();
    struct instruction_set is2;
    instruction_set_init(&is2);
    struct clear_actions clear;
    inst_clear_actions(&clear);
    add_clear_actions(&is2, clear);
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    flow_add_instructions(fl3, is2);
    modify_flow(ft, fl3, false, 0);
    /* Check if the two flows were modified.*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct ofl_flow_key), ret);
        /* Check if the instruction active fields are equal to the flow mod */
        if (ret) {
            assert_int_equal(ret->insts.active, is2.active);
        }
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct ofl_flow_key), ret);
        if (ret) {
            assert_int_equal(ret->insts.active, is2.active);
        }
    }
    flow_destroy(fl3);
    flow_table_destroy(ft);
}

void stress_modify_non_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct ofl_flow_key *keys = malloc(sizeof(struct ofl_flow_key) * 1000000);
    int i;
    for (i = 0; i < 1000000; ++i) {
        struct flow *fl = flow_new();
        struct instruction_set is;
        instruction_set_init(&is);
        struct goto_table gt;
        inst_goto_table(&gt, 1);
        add_goto_table(&is, gt);
        struct write_metadata wm;
        inst_write_metadata(&wm, 0xbeef, 0xffffffffffffffff);
        add_write_metadata(&is, wm);
        set_ip_proto(fl, 7);
        set_eth_type(fl, 0x800);
        set_ipv4_dst(fl, i);
        memset(&keys[i], 0x0, sizeof(struct ofl_flow_key));
        keys[i].ip_proto = 7;
        keys[i].eth_type = 0x800;
        keys[i].ipv4_dst = i;
        flow_add_instructions(fl, is);
        add_flow(ft, fl, 0);
    }

    /* Flow to be modified */
    struct flow *fl3 = flow_new();
    struct instruction_set is2;
    instruction_set_init(&is2);
    struct clear_actions clear;
    inst_clear_actions(&clear);
    add_clear_actions(&is2, clear);
    flow_add_instructions(fl3, is2);
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    modify_flow(ft, fl3, false, 0);
    /* Check if all flows were modified*/
    i = 0;
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &keys[i], sizeof(struct ofl_flow_key), ret);
        if (ret) {
            assert_int_equal(ret->insts.active, is2.active);
        }
        i++;
    }
    free(keys);
    flow_destroy(fl3);
    flow_table_destroy(ft);
}

/* Succeed if instruction two flows are gone */
void delete_non_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    struct instruction_set is;
    instruction_set_init(&is);
    struct goto_table gt;
    inst_goto_table(&gt, 1);
    add_goto_table(&is, gt);
    // struct write_metadata *wm = inst_new_write_metadata(0xbeef);
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    flow_add_instructions(fl, is);
    add_flow(ft, fl, 0);
    struct flow *fl2 = flow_new();
    set_ip_ecn(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    add_flow(ft, fl2, 0);
    /* Flow to be deleted */
    struct flow *fl3 = flow_new();
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    delete_flow(ft, fl3, 0, false);
    /* Check if both flows were modified*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct ofl_flow_key), ret);
        assert_int_equal(ret, NULL);
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct ofl_flow_key), ret);
        assert_int_equal(ret, NULL);
    }
    flow_destroy(fl3);
    flow_table_destroy(ft);
}

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(lookup),
        unit_test(lookup_expired),
        unit_test(lookup_priority),
        unit_test(add_single_flow),
        unit_test(add_flows_same_field_type),
        unit_test(add_flows_diff_field_type),
        unit_test(modify_strict),
        unit_test(modify_strict_expired),
        unit_test(modify_non_strict),
        unit_test(delete_non_strict),
    };
    return run_tests(tests);
}