#include "action.h"
#include "util.h"

void 
action_set_init(struct action_set *as)
{
    int i;
    for (i = 0; i < MAX_ACTION_SET; ++i){
        as->actions[i] = NULL;
    }
}

// static void action_cast

void action_set_add(struct action_set *as, struct action_header *act, uint8_t type){
    as->actions[type] = act;
};

struct output* action_new_output(uint32_t port){
    struct output *out = xmalloc(sizeof(struct output));
    out->header.type = ACT_OUTPUT;
    out->port = port;
    return out;
}