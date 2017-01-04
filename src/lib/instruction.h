#ifndef INSTRUCTION_H
#define INSTRUCTION_H 1

#include <inttypes.h>

#define INST_MAX 5

enum instruction_type {
    INSTRUCTION_APPLY_ACTIONS,
    INSTRUCTION_CLEAR_ACTIONS,
    INSTRUCTION_WRITE_ACTIONS,
    INSTRUCTION_WRITE_METADATA,
    INSTRUCTION_GOTO_TABLE
};

struct inst_header {
    uint8_t type;
};

struct apply_actions {
    struct inst_header hdr;
};

struct write_actions {
    struct inst_header hdr;
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
struct inst_header* inst_new_clear_actions(void);
struct write_actions* inst_new_write_actions(void);
struct write_metadata* inst_new_write_metadata(uint64_t metadata);
struct goto_table* inst_new_goto_table(uint8_t table_id);

#endif