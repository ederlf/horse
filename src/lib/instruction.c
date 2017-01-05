#include "instruction.h"
#include "util.h"

struct apply_actions* 
inst_new_apply_actions(void)
{
    struct apply_actions *aa = xmalloc(sizeof(struct apply_actions));
    aa->hdr.type = INSTRUCTION_APPLY_ACTIONS;
    return aa;
}

void
set_clear_actions(struct clear_actions *ca)
{
    ca->hdr.type = INSTRUCTION_CLEAR_ACTIONS;
}

struct write_actions* 
inst_new_write_actions(void)
{
    struct write_actions *wa = xmalloc(sizeof(struct write_actions));
    wa->hdr.type = INSTRUCTION_WRITE_ACTIONS;
    return wa;
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