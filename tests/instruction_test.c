#include "lib/flow.h"
#include "lib/instruction.h"
#include <uthash/utlist.h>
#include "cmockery_horse.h"

void apply_actions(void **state)
{
    struct flow *fl = flow_new();
    struct instruction_set is;
    instruction_set_init(&is);
    set_eth_type(fl, 0x800);
    struct apply_actions aa;
    struct action_list al;
    struct action gen_act;
    action_list_init(&al);
    action_output(&gen_act, 2);
    action_list_add(&al, gen_act);
    action_set_field_u16(&gen_act, SET_IP_PROTO, 6);
    action_list_add(&al, gen_act);
    inst_apply_actions(&aa, al);
    add_apply_actions(&is, aa);
    flow_add_instructions(fl, is);
    assert_int_equal(fl->insts.active, INSTRUCTION_APPLY_ACTIONS);
    int count = 0;
    struct action_list_elem *elem;
    LL_COUNT(fl->insts.apply_act.actions.actions, elem, count);
    assert_int_equal(count, 2);
    flow_destroy(fl);
}

void write_actions(void **state)
{
    struct flow *fl = flow_new();
    struct instruction_set is;
    instruction_set_init(&is);
    set_eth_type(fl, 0x800);
    struct write_actions wa;
    struct action_set as;
    struct action gen_act;
    action_set_init(&as);
    action_output(&gen_act, 2);
    action_set_add(&as, gen_act);
    inst_write_actions(&wa, as);
    add_write_actions(&is, wa);
    flow_add_instructions(fl, is);
    assert_int_equal(fl->insts.active, INSTRUCTION_WRITE_ACTIONS);
    int count = 0;
    struct action_set_elem *elem;
    LL_COUNT(fl->insts.write_act.actions.actions, elem, count);
    assert_int_equal(count, 1);
    flow_destroy(fl);
}

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(apply_actions),
        unit_test(write_actions),
    };
    return run_tests(tests);
}