#ifndef INSTRUCTION_H
#define INSTRUCTION_H 1

#include "action_set.h"
#include <inttypes.h>

#define INST_MAX 5

enum instruction_type {
    INSTRUCTION_APPLY_ACTIONS  =  1 << 0,
    INSTRUCTION_CLEAR_ACTIONS  =  1 << 1,
    INSTRUCTION_WRITE_ACTIONS  =  1 << 2,
    INSTRUCTION_WRITE_METADATA =  1 << 3,
    INSTRUCTION_GOTO_TABLE     =  1 << 4,
};

struct inst_header {
    uint8_t type;
};

struct apply_actions {
    struct inst_header hdr;
    struct action_list actions;
};

struct clear_actions {
    struct inst_header hdr;  
};

struct write_actions {
    struct inst_header hdr;
    struct action_set actions;
};

struct write_metadata {
    struct inst_header hdr;
    uint64_t metadata;
};

struct goto_table {
    struct inst_header hdr;
    uint8_t table_id;
};

struct apply_actions* inst_new_apply_actions(void);
void set_clear_actions(struct clear_actions *ca);
struct write_actions* inst_new_write_actions(void);
void set_write_metadata(struct write_metadata *wm, uint64_t metadata);
void set_goto_table(struct goto_table *gt, uint8_t table_id);

#endif