#include "net/flow_table.h"
#include <uthash/utlist.h>
#include "cmockery_horse.h"

void stress_stack(void **state)
{
    struct flow_table ft[1000000];
    // memset(ft, 0x0, sizeof(struct flow_table) * 128);
    printf("%lu\n", sizeof(ft));
}

/* Test succeed if flow can be found in the table */
void add_single_flow(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    add_flow(ft, fl);
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        unsigned int num_flows = HASH_COUNT(hash);
        assert_int_equal(num_flows, 1);
        HASH_FIND(hh, hash, &fl->key, sizeof(struct flow_key), ret);
        if (ret){
            /* Key was checked in the hash find...*/
            assert_int_equal(ret->key.eth_type, 0x800);
        }  
    }
    flow_table_clean(ft);
}

/* Test succeeds if there is ONE mini flow table and TWO flows */
void add_flows_same_field_type(void **state)
{
    struct flow *ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    set_eth_type(fl, 0x800);
    add_flow(ft, fl);
    struct flow *fl2 = flow_new();
    set_eth_type(fl2, 0x806);
    add_flow(ft, fl2);
    int count;
    DL_COUNT(ft->flows, elt, count);
    assert_int_equal(count, 1);
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        unsigned int num_flows = HASH_COUNT(hash);
        assert_int_equal(num_flows, 2);
    }
    flow_table_clean(ft);
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
    add_flow(ft, fl);
    struct flow *fl2 = flow_new();
    set_eth_type(fl2, 0x806);
    add_flow(ft, fl2);
    int count;
    DL_COUNT(ft->flows, elt, count);
    assert_int_equal(count, 2);
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        unsigned int num_flows = HASH_COUNT(hash);
        assert_int_equal(num_flows, 1);
    }
    flow_table_clean(ft);
}

/* Succeed if instruction for a single flow is modified */
void modify_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    struct goto_table *gt = inst_new_goto_table(1);
    struct write_metadata *wm = inst_new_write_metadata(0xbeef);
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    flow_add_instruction(fl, (struct inst_header*) gt);
    add_flow(ft, fl);
    struct flow *fl2 = flow_new();
    set_ipv4_dst(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    flow_add_instruction(fl2, (struct inst_header*) wm);
    add_flow(ft, fl2);
    /* Flow to be modified */
    struct flow *fl3 = flow_new();
    struct inst_header *clean = inst_new_clear_actions();
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    flow_add_instruction(fl3, clean);
    modify_flow(ft, fl3, true);
    /* Check if only the first flow is modified*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->instructions[INSTRUCTION_CLEAR_ACTIONS]->type, INSTRUCTION_CLEAR_ACTIONS);
        }
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->instructions[INSTRUCTION_WRITE_METADATA]->type, INSTRUCTION_WRITE_METADATA);
        }  
    }
    flow_destroy(fl3);
    flow_table_clean(ft);
}

/* Succeed if instruction for two flows is modified */
void modify_non_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    struct goto_table *gt = inst_new_goto_table(1);
    struct write_metadata *wm = inst_new_write_metadata(0xbeef);
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    flow_add_instruction(fl, (struct inst_header*) gt);
    add_flow(ft, fl);
    struct flow *fl2 = flow_new();
    set_ipv4_dst(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    flow_add_instruction(fl, (struct inst_header*) wm);
    add_flow(ft, fl2);
    /* Flow to be modified */
    struct flow *fl3 = flow_new();
    struct inst_header *clean = inst_new_clear_actions();
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    flow_add_instruction(fl3, (struct inst_header*) clean);
    modify_flow(ft, fl3, false);
    /* Check if both flows were modified*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->instructions[INSTRUCTION_CLEAR_ACTIONS]->type, INSTRUCTION_CLEAR_ACTIONS);
        }
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->instructions[INSTRUCTION_CLEAR_ACTIONS]->type, INSTRUCTION_CLEAR_ACTIONS);
        }  
    }
    flow_destroy(fl3);
    flow_table_clean(ft);
}

void stress_modify_non_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow_key *keys = malloc(sizeof(struct flow_key) * 1000000);
    int i;
    for (i = 0; i < 1000000; ++i){
        struct flow *fl = flow_new();
        struct goto_table *gt = inst_new_goto_table(1);
        struct write_metadata *wm = inst_new_write_metadata(0xbeef); 
        set_ip_proto(fl, 7);
        set_eth_type(fl, 0x800);
        set_ipv4_dst(fl, i);
        memset(&keys[i],0x0, sizeof(struct flow_key));
        keys[i].ip_proto = 7;
        keys[i].eth_type = 0x800;
        keys[i].ipv4_dst = i;
        flow_add_instruction(fl, (struct inst_header*) gt);
        flow_add_instruction(fl, (struct inst_header*) wm);
        add_flow(ft, fl);
    }

    /* Flow to be modified */
    struct flow *fl3 = flow_new();
    struct inst_header *clean = inst_new_clear_actions();
    struct apply_actions *apply = inst_new_apply_actions();
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    flow_add_instruction(fl3, clean);
    flow_add_instruction(fl3, (struct inst_header*) apply);
    modify_flow(ft, fl3, false);
    /* Check if all flows were modified*/
    i = 0;
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &keys[i], sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->instructions[INSTRUCTION_GOTO_TABLE], NULL);
            assert_int_equal(ret->instructions[INSTRUCTION_WRITE_METADATA], NULL);
            assert_int_equal(ret->instructions[INSTRUCTION_CLEAR_ACTIONS]->type, INSTRUCTION_CLEAR_ACTIONS);
            assert_int_equal(ret->instructions[INSTRUCTION_APPLY_ACTIONS]->type, INSTRUCTION_APPLY_ACTIONS);
        }
        i++; 
    }
    free(keys);
    flow_destroy(fl3);
    flow_table_clean(ft);
}

/* Succeed if instruction two flows are gone */
void delete_non_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new(0);
    struct flow *fl = flow_new();
    struct goto_table *gt = inst_new_goto_table(1);
    // struct write_metadata *wm = inst_new_write_metadata(0xbeef);
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    flow_add_instruction(fl, (struct inst_header*) gt);
    add_flow(ft, fl);
    struct flow *fl2 = flow_new();
    set_ipv4_dst(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    flow_add_instruction(fl, (struct inst_header*) gt);
    add_flow(ft, fl2);
    /* Flow to be deleted */
    struct flow *fl3 = flow_new();
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    delete_flow(ft, fl3, false);
    /* Check if both flows were modified*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct flow_key), ret);
        assert_int_equal(ret, NULL);
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct flow_key), ret);
        assert_int_equal(ret, NULL);
    }
    flow_destroy(fl3);
    flow_table_clean(ft);
}

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(add_single_flow),
        unit_test(add_flows_same_field_type),
        unit_test(add_flows_diff_field_type),
        unit_test(modify_strict),
        unit_test(modify_non_strict),
        unit_test(delete_non_strict),
        unit_test(stress_modify_non_strict),
    };
    return run_tests(tests);
}