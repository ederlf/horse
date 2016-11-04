#include "net/flow_table.h"
#include <uthash/utlist.h>
#include "cmockery_horse.h"


/* Test succeed if flow can be found in the table */
void add_single_flow(void **state)
{
    struct flow * ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
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
    flow_table_destroy(ft);
}

/* Test succeeds if there is ONE mini flow table and TWO flows */
void add_flows_same_field_type(void **state)
{
    struct flow * ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
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
    flow_table_destroy(ft);
}

/*  Test succeeds if there are TWO mini flow table 
*   and ONE flow in each mini table. 
*/
void add_flows_diff_field_type(void **state)
{
    struct flow * ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
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
    flow_table_destroy(ft);
}

/* Succeed if action for two flows is modified */
void modify_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
    struct flow *fl = flow_new();
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    fl->action = 1;
    add_flow(ft, fl);
    struct flow *fl2 = flow_new();
    set_ipv4_dst(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    add_flow(ft, fl2);
    fl2->action = 2;
    struct flow *fl3 = flow_new();
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    fl3->action = 3;
    modify_flow(ft, fl3, true);
    /* Check if only the first flow is modified*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->action, fl3->action);
        }
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->action, fl2->action);
        }  
    }
    free_flow(fl3);
    flow_table_destroy(ft);
}

/* Succeed if action for two flows is modified */
void modify_non_strict(void **state)
{
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
    struct flow *fl = flow_new();
    set_ip_proto(fl, 7);
    set_eth_type(fl, 0x800);
    fl->action = 1;
    add_flow(ft, fl);
    struct flow *fl2 = flow_new();
    set_ipv4_dst(fl2, 21);
    set_eth_type(fl2, 0x800);
    set_ip_proto(fl2, 7);
    add_flow(ft, fl2);
    fl2->action = 2;
    struct flow *fl3 = flow_new();
    set_ip_proto(fl3, 7);
    set_eth_type(fl3, 0x800);
    fl3->action = 3;
    modify_flow(ft, fl3, false);
    /* Check if both flows were modified*/
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        struct flow* ret;
        HASH_FIND(hh, hash, &fl->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->action, 3);
        }
        HASH_FIND(hh, hash, &fl2->key, sizeof(struct flow_key), ret);
        if (ret){
            assert_int_equal(ret->action, 3);
        }  
    }
    free_flow(fl3);
    flow_table_destroy(ft);
}

            

// Lookup
// struct flow_table *ft = flow_table_new();
//                 struct flow *fl = flow_new();
//                 set_ip_proto(fl, 7);
//                 set_eth_type(fl, 0x800);
//                 fl->priority = 1;
//                 fl->action = 1;
//                 add_flow(ft, fl);
//                 struct flow *fl2 = flow_new();
//                 set_ipv4_dst(fl2, 21);
//                 set_eth_type(fl2, 0x800);
//                 set_ip_proto(fl2, 7);
//                 add_flow(ft, fl2);
//                 fl2->priority = 10;
//                 fl2->action = 2;
//                 add_flow(ft, fl2);
//                 struct flow *fl3 = flow_new();
//                 set_ip_proto(fl3, 7);
//                 set_eth_type(fl3, 0x806);
//                 fl3->action = 3;
//                 add_flow(ft, fl3);
//                 struct flow *fl4 = flow_new();
//                 set_ipv4_dst(fl4, 21);
//                 set_ip_proto(fl4, 7);
//                 set_eth_type(fl4, 0x800);
//                 // modify_flow(ft, fl3, false);
//                 // delete_flow(ft, fl3, true);
//                 struct mini_flow_table *elt;
//                 int count;
//                 DL_COUNT(ft->flows, elt, count);
//                 printf("%d\n", count); 
//                 struct flow *ret = flow_table_lookup(ft, fl4);
//                 if (ret){
//                     printf("Action %d\n", ret->action);
//                 }


int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(add_single_flow),
        unit_test(add_flows_same_field_type),
        unit_test(add_flows_diff_field_type),
        unit_test(modify_strict),
        unit_test(modify_non_strict),
    };
    return run_tests(tests);
}