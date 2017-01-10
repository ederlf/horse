#include "instruction_set.h"
#include "util.h"

void 
instruction_set_init(struct instruction_set *is)
{
    is->active = 0;
    action_list_init(&is->apply_act.actions);
    action_set_init(&is->write_act.actions);
}

void 
add_apply_actions(struct instruction_set *is, struct apply_actions act)
{
    is->active |=  INSTRUCTION_APPLY_ACTIONS;
    memcpy(&is->apply_act, &act, sizeof(struct apply_actions));
}

void 
add_clear_actions(struct instruction_set *is, struct clear_actions act)
{
    is->active |=  INSTRUCTION_CLEAR_ACTIONS;
    memcpy(&is->clear_act, &act, sizeof(struct goto_table));
}

void 
add_write_actions(struct instruction_set *is, struct write_actions act)
{
    is->active |=  INSTRUCTION_WRITE_ACTIONS;
    memcpy(&is->write_act, &act, sizeof(struct write_actions));
}

void 
add_write_metadata(struct instruction_set *is, struct write_metadata act)
{
    is->active |=  INSTRUCTION_WRITE_METADATA; 
    memcpy(&is->write_meta, &act, sizeof(struct write_metadata));
}

void 
add_goto_table(struct instruction_set *is, struct goto_table act)
{
    is->active |=  INSTRUCTION_GOTO_TABLE; 
    memcpy(&is->gt_table, &act, sizeof(struct goto_table));
}

void 
instruction_set_clean(struct instruction_set *is)
{   
    apply_actions_clean(&is->apply_act);
    write_actions_clean(&is->write_act);
}