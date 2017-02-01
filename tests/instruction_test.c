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
    unsigned int actions_num;
    actions_num = HASH_COUNT(fl->insts.write_act.actions.actions);
    assert_int_equal(actions_num, 1);
    flow_destroy(fl);
}

void merge_action_set(void **state)
{
    struct action_set as1;
    struct action_set as2;
    struct action gen_act;
    action_set_init(&as1);
    action_set_init(&as2);
    action_output(&gen_act, 2);
    action_set_add(&as1, gen_act);
    action_set_field_u16(&gen_act, SET_IP_PROTO, 6);
    action_set_add(&as1, gen_act);
    action_output(&gen_act, 1);
    action_set_add(&as2, gen_act);
    action_push_vlan(&gen_act, 0x800);
    action_set_add(&as2, gen_act);
    action_set_merge(&as1, &as2);
    unsigned int actions_num;
    actions_num = HASH_COUNT(as1.actions);
    printf("%d\n", actions_num);
    struct action *act;
    uint16_t type = ACT_PUSH_VLAN;
    HASH_FIND(hh, as1.actions, &type, sizeof(uint16_t), act);
    assert_int_not_equal(act, NULL);
    type = ACT_SET_FIELD;
    HASH_FIND(hh, as1.actions, &type, sizeof(uint16_t), act);
    assert_int_not_equal(act, NULL);
    type = ACT_OUTPUT;
    HASH_FIND(hh, as1.actions, &type, sizeof(uint16_t), act);
    assert_int_not_equal(act, NULL);
    assert_int_equal(act->out.port, 1);
    action_set_clean(&as1);
    actions_num = HASH_COUNT(as2.actions);
    action_set_clean(&as2);
}

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(apply_actions),
        unit_test(write_actions),
        unit_test(merge_action_set),
    };
    return run_tests(tests);
}