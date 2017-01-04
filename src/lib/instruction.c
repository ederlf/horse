#include "instruction.h"
#include "util.h"

struct apply_actions* 
inst_new_apply_actions(void){
    struct apply_actions *aa = xmalloc(sizeof(struct apply_actions));
    aa->hdr.type = INSTRUCTION_APPLY_ACTIONS;
    return aa;
}

struct inst_header* 
inst_new_clear_actions(void){
    struct inst_header *inst = xmalloc(sizeof(struct inst_header));
    inst->type = INSTRUCTION_CLEAR_ACTIONS;
    return inst;
}

struct write_actions* 
inst_new_write_actions(void)
{
    struct write_actions *wa = xmalloc(sizeof(struct write_actions));
    wa->hdr.type = INSTRUCTION_WRITE_ACTIONS;
    return wa;
}

struct write_metadata* 
inst_new_write_metadata(uint64_t metadata)
{
    struct write_metadata *wm = xmalloc(sizeof(struct write_metadata));
    wm->hdr.type = INSTRUCTION_WRITE_METADATA;
    wm->metadata = metadata;
    return wm;
}

struct goto_table* 
inst_new_goto_table(uint8_t table_id){
    struct goto_table *gt = xmalloc(sizeof(struct goto_table));
    gt->hdr.type = INSTRUCTION_GOTO_TABLE;
    gt->table_id = table_id;
    return gt;
}