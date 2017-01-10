#ifndef INSTRUCTION_SET_H

#include "instruction.h"
#include <inttypes.h>

struct instruction_set {
    uint8_t active;           /* Bitmap of active instructions */
    struct apply_actions apply_act;
    struct clear_actions clear_act;
    struct write_actions write_act;
    struct write_metadata write_meta;
    struct goto_table gt_table;
};

// struct instruction_set* instruction_set_new(void);
void add_apply_actions(struct instruction_set *is, struct apply_actions act);
void add_clear_actions(struct instruction_set *is, struct clear_actions act);
void add_write_actions(struct instruction_set *is, struct write_actions act);
void add_write_metadata(struct instruction_set *is, struct write_metadata act);
void add_goto_table(struct instruction_set *is, struct goto_table act);

#define INSTRUCTION_SET_H 1
#endif