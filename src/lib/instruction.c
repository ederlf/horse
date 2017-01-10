#include "instruction.h"
#include "util.h"

void 
set_apply_actions(struct apply_actions *aa, struct action_list al)
{
    aa->hdr.type = INSTRUCTION_APPLY_ACTIONS;
    aa->actions = al;
}

void
set_clear_actions(struct clear_actions *ca)
{
    ca->hdr.type = INSTRUCTION_CLEAR_ACTIONS;
}

void 
set_write_actions(struct write_actions *wa, struct action_set as)
{
    wa->hdr.type = INSTRUCTION_WRITE_ACTIONS;
    wa->actions = as;
}

void
set_write_metadata(struct write_metadata *wm, uint64_t metadata)
{
    wm->hdr.type = INSTRUCTION_WRITE_METADATA;
    wm->metadata = metadata;
}

void
set_goto_table(struct goto_table *gt, uint8_t table_id){
    gt->hdr.type = INSTRUCTION_GOTO_TABLE;
    gt->table_id = table_id;
}