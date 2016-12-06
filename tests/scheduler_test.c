#include "sim/scheduler.h"
#include <uthash/utlist.h>
#include <time.h>
#include "cmockery_horse.h"

int cmpfunc (const void * a, const void * b)
{
   return ( *(uint64_t*)a - *(uint64_t*)b );
}

/* Tests if the event order is correct */
void add_flow_event(void **state){
    struct scheduler* sch = scheduler_new();
    struct event ev_ret;
    struct event ev[10];
    uint64_t times[10] = {50, 80, 70, 60, 40, 90, 10, 30, 100, 20};
    int i;
    for (i = 0; i < 10; ++i){
        init_event(&ev[i], EVENT_FLOW, times[i], i + 1);    
        scheduler_insert(sch, &ev[i]);
    } 
    qsort(times, 10, sizeof(uint64_t), cmpfunc);
    for (i = 0; i < 10; ++i){
        ev_ret = scheduler_dispatch(sch);
        assert_int_equal(ev_ret.time, times[i]);        
    } 
    scheduler_destroy(sch);
}

/* Generates random times and checks if the order is correct */
void random_add_flow_event(void **state){
    struct scheduler* sch = scheduler_new();
    struct event ev_ret;
    struct event ev[100000];
    uint64_t times[100000];
    int i;
    srand(time(NULL));
    for (i = 0; i < 100000; ++i){
        times[i] =  rand(); 
        init_event(&ev[i], EVENT_FLOW, times[i], i + 1);    
        scheduler_insert(sch, &ev[i]);
    } 
    qsort(times, 100000, sizeof(uint64_t), cmpfunc);
    for (i = 0; i < 100000; ++i){
        ev_ret = scheduler_dispatch(sch);
        assert_int_equal(ev_ret.time, times[i]);        
    } 
    scheduler_destroy(sch);
}

void event_table(void **state){
    struct event_hdr *table = NULL;
    struct event_hdr *tmp, *p;
    struct event_flow *flow = malloc(sizeof(struct event_flow));
    uint64_t id = 1;
    flow->hdr.id = 1;
    flow->hdr.type = EVENT_FLOW;
    flow->dpid = 0x9000000000000001;
    HASH_ADD(hh, table, id, sizeof(uint64_t), (struct event_hdr*) flow);

    HASH_FIND(hh, table, &id, sizeof(uint64_t), p);

    if (p){
        struct event_flow *f = (struct event_flow*)p;
        printf("%lx\n", f->dpid);
    }

    HASH_ITER(hh, table, p, tmp) {
      HASH_DEL(table, p);
      free(p);
    }
}

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        // unit_test(add_flow_event),
        unit_test(event_table),
    };
    return run_tests(tests);
}