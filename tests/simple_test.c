#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <google/cmockery.h>

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

// A test case that does nothing and succeeds.
void null_test_success(void **state) {
}

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(null_test_success),
    };
    return run_tests(tests);
}