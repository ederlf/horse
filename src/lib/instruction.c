#include "instruction.h"
#include "util.h"

void
inst_apply_actions(struct apply_actions *aa, struct action_list al)
{
    aa->hdr.type = INSTRUCTION_APPLY_ACTIONS;
    aa->actions = al;
}

void
inst_clear_actions(struct clear_actions *ca)
{
    ca->hdr.type = INSTRUCTION_CLEAR_ACTIONS;
}

void
inst_write_actions(struct write_actions *wa, struct action_set as)
{
    wa->hdr.type = INSTRUCTION_WRITE_ACTIONS;
    wa->actions = as;
}

void
inst_write_metadata(struct write_metadata *wm, uint64_t metadata,
                    uint64_t metadata_mask)
{
    wm->hdr.type = INSTRUCTION_WRITE_METADATA;
    wm->metadata = metadata;
    wm->metadata_mask = metadata_mask;
}

void
inst_goto_table(struct goto_table *gt, uint8_t table_id) {
    gt->hdr.type = INSTRUCTION_GOTO_TABLE;
    gt->table_id = table_id;
}

void
apply_actions_clean(struct apply_actions *aa) {
    action_list_clean(&aa->actions);
}

void
write_actions_clean(struct write_actions *wa)
{
    action_set_clean(&wa->actions);
}