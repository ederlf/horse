#include "net/flow_table.h"
#include <uthash/utlist.h>
#include "cmockery_horse.h"


/* Test succeed if flow can be found in the table */
void add_single_flow(void **state)
{
    struct flow * ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
    struct flow *fl = new_flow();
    set_eth_type(fl, 0x800);
    add_flow(ft, fl);
    DL_FOREACH(ft->flows, elt) {
        struct flow *hash = elt->flows;
        unsigned int num_flows = HASH_COUNT(hash);
        assert_int_equal(num_flows, 1);
        HASH_FIND(hh, hash, &fl->key, sizeof(struct flow_key), ret);
        if (ret){
            /* Need to check the whole flow. Use flow key cmp*/
            assert_int_equal(ret->key.eth_type, 0x800);
        }  
    }
    flow_table_destroy(ft);
}

/* Test succeeds if there is one mini flow table and two flows */
void add_flows_same_field_type(void **state)
{
    struct flow * ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
    struct flow *fl = new_flow();
    set_eth_type(fl, 0x800);
    add_flow(ft, fl);
    struct flow *fl2 = new_flow();
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

/* Test succeeds if there are two mini flow table and one flow in each */
void add_flows_diff_field_type(void **state)
{
    struct flow * ret;
    struct mini_flow_table *elt;
    struct flow_table *ft = flow_table_new();
    struct flow *fl = new_flow();
    set_eth_type(fl, 0x800);
    set_ip_proto(fl, 17);
    add_flow(ft, fl);
    struct flow *fl2 = new_flow();
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

// Add
// struct flow_table *ft = flow_table_new();
// struct flow *fl = new_flow();
// set_eth_type(fl, 0x800);
// add_flow(ft, fl);
// struct flow *fl2 = new_flow();
// set_ip_proto(fl2, 17);
// add_flow(ft, fl2);
// struct flow *fl3 = new_flow();
// set_eth_type(fl3, 0x806);
// add_flow(ft, fl3);
// struct mini_flow_table *elt;
// int count;
// DL_COUNT(ft->flows, elt, count);
// printf("%d\n", count);
// DL_FOREACH(ft->flows, elt) {
//     struct flow *hash = elt->flows;
//     unsigned int num_flows;
//     num_flows = HASH_COUNT(hash);
//     printf("%u\n", num_flows);
//     struct flow *f = new_flow();
//     set_eth_type(f, 0x806);
//     struct flow* ret;
//     HASH_FIND(hh, hash, &f->key, sizeof(struct flow_key), ret);
//     if (ret){
//         printf("CHIUBACA!\n");
//     }                                
//     struct flow *f2 = new_flow();
//     set_ip_proto(f2, 17);
//     HASH_FIND(hh, hash, &f2->key, sizeof(struct flow_key), ret);
//     if (ret){
//         printf("CHIUBACA 2!\n");
//     }
// }

// Modify/Delete
// struct flow_table *ft = flow_table_new();
//                 struct flow *fl = new_flow();
//                 set_ip_proto(fl, 7);
//                 set_eth_type(fl, 0x800);
//                 fl->action = 1;
//                 add_flow(ft, fl);
//                 struct flow *fl2 = new_flow();
//                 set_ipv4_dst(fl2, 21);
//                 set_eth_type(fl2, 0x800);
//                 set_ip_proto(fl2, 7);
//                 add_flow(ft, fl2);
//                 fl2->action = 2;
//                 struct flow *fl3 = new_flow();
//                 set_ip_proto(fl3, 7);
//                 set_eth_type(fl3, 0x800);
//                 fl3->action = 3;
//                 // add_flow(ft, fl3);
//                 modify_flow(ft, fl3, false);
//                 struct mini_flow_table *elt;
//                 // int count;
//                 // DL_COUNT(ft->flows, elt, count);
//                 // printf("%d\n", count); 
//                 DL_FOREACH(ft->flows, elt) {
//                     struct flow *hash = elt->flows;
//                     unsigned int num_flows;
//                     num_flows = HASH_COUNT(hash);
//                     printf("%u\n", num_flows);
//                     struct flow *f = new_flow();
//                     set_eth_type(f, 0x800);
//                     set_ip_proto(f, 7);
//                     struct flow* ret;
//                     HASH_FIND(hh, hash, &f->key, sizeof(struct flow_key), ret);
//                     if (ret){
//                         printf("Modify first %d!\n", ret->action);
//                     }                                
//                     struct flow *f2 = new_flow();
//                     // set_ip_proto(f2, 17);
//                     set_ipv4_dst(f2, 21);
//                     set_eth_type(f2, 0x800);
//                     set_ip_proto(f2, 7);
//                     HASH_FIND(hh, hash, &f2->key, sizeof(struct flow_key), ret);
//                     if (ret){
//                         printf("Mod second %d!\n", ret->action);
//                     }
//                 }               


// Lookup
// struct flow_table *ft = flow_table_new();
//                 struct flow *fl = new_flow();
//                 set_ip_proto(fl, 7);
//                 set_eth_type(fl, 0x800);
//                 fl->priority = 1;
//                 fl->action = 1;
//                 add_flow(ft, fl);
//                 struct flow *fl2 = new_flow();
//                 set_ipv4_dst(fl2, 21);
//                 set_eth_type(fl2, 0x800);
//                 set_ip_proto(fl2, 7);
//                 add_flow(ft, fl2);
//                 fl2->priority = 10;
//                 fl2->action = 2;
//                 add_flow(ft, fl2);
//                 struct flow *fl3 = new_flow();
//                 set_ip_proto(fl3, 7);
//                 set_eth_type(fl3, 0x806);
//                 fl3->action = 3;
//                 add_flow(ft, fl3);
//                 struct flow *fl4 = new_flow();
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
    };
    return run_tests(tests);
}